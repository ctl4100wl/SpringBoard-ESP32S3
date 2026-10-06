/**
 * @file lens.c
 * "Liquid glass": a frosted pane whose rim refracts the content behind it.
 *
 * How it works (cheap enough for an ESP32-S3, no GPU):
 *  1. The pane is an ordinary backdrop-blur object (ui_glass), so the interior is a mild frost.
 *  2. Only a narrow bezel band along the edge refracts. For every band pixel a displacement vector is
 *     computed ONCE per pane size: distance to the rounded-rect edge -> convex "squircle" height profile
 *     -> slope -> Snell's law (glass index 1.5) -> sideways shift. The result is a small look-up table
 *     (3 bytes per band pixel: dx, dy, specular), ~40 KB for a full-width pill.
 *  3. Each frame, a hook around LVGL's software blur task (lv_draw_sw_blur_hook, see
 *     patches/lvgl-blur-hook.patch) copies the pane's backdrop before the frost, then rewrites the band
 *     pixels from the *sharp* copy at the displaced position (optionally with the red / blue channels
 *     displaced a little differently = chromatic fringe) and adds a specular rim highlight.
 *  Per frame cost = one memcpy of the pane + a gather per band pixel (a few thousand).
 */
#define S3_NATIVE_DRIVER 1
#include <math.h>
#include <string.h>
#include "sdk.h"
#include "lvgl/src/draw/lv_draw_private.h"
#include "lvgl/src/misc/lv_area_private.h"

#if OS_FX_LENS

#define LENS_MAX    12

extern void (*lv_draw_sw_blur_hook)(lv_draw_task_t * t, const lv_draw_blur_dsc_t * dsc, const lv_area_t * coords,
                                    bool post);

typedef struct {
    lv_obj_t * obj;
    int32_t bezel;          /* band width in px */
    int32_t strength;       /* 0..100: glass thickness relative to the bezel */
    bool chroma;
    int8_t * lut;           /* dx, dy, spec per band pixel, in scan order */
    int32_t lw, lh, lr;     /* size the table was built for */
    uint8_t * scratch;      /* sharp copy of the backdrop (tightly packed rows) */
    size_t scratch_cap;
    lv_area_t copy;         /* area of the copy, global coordinates */
    int32_t px;             /* bytes per pixel when copied */
} lens_t;

static lens_t s_lens[LENS_MAX];

static lens_t * find(lv_obj_t * obj)
{
    for(int i = 0; i < LENS_MAX; i++) if(s_lens[i].obj == obj) return &s_lens[i];
    return NULL;
}

/* Interior (not refracting) x-range [x0, x1) of row y: the shape inset by the bezel. Empty if the band covers the row. */
static void inner_span(int32_t y, int32_t w, int32_t h, float rr, int32_t b, int32_t * x0, int32_t * x1)
{
    float hx = w * 0.5f, hy = h * 0.5f;
    float ri = LV_MAX(rr - (float)b, 0.0f);
    float py = fabsf(y + 0.5f - hy);
    float qy = py - (hy - (float)b - ri);
    float half;
    if(py >= hy - (float)b) half = -1.0f;
    else if(qy <= 0.0f) half = hx - (float)b;
    else if(qy < ri) half = hx - (float)b - ri + sqrtf(ri * ri - qy * qy);
    else half = -1.0f;
    if(half <= 0.0f) {
        *x0 = *x1 = 0;
        return;
    }
    *x0 = (int32_t)ceilf(hx - half);
    *x1 = (int32_t)floorf(hx + half);
}

/* Visit every band pixel (distance to the edge < bezel, or in the corner cut-outs) in a fixed scan order */
#define BAND_FOREACH(w, h, rr, b, ...)                                                       for(int32_t y_ = 0; y_ < (h); y_++) {                                                        int32_t i0_, i1_;                                                                        inner_span(y_, (w), (h), (rr), (b), &i0_, &i1_);                                         for(int32_t x_ = 0; x_ < (w); x_++) {                                                        if(x_ >= i0_ && x_ < i1_) { x_ = i1_ - 1; continue; }                                    __VA_ARGS__                                                                          }                                                                                    }

extern float lens_shift_at(float u, float thick);   /* glass_text.c */

static bool lut_build(lens_t * l, int32_t w, int32_t h, int32_t r)
{
    int32_t b = LV_MIN(l->bezel, LV_MIN(w, h) / 2);
    float rr0 = LV_MIN((float)r, LV_MIN(w, h) * 0.5f);
    size_t n = 0;
    BAND_FOREACH(w, h, rr0, b, n++;)
    int8_t * lut = lv_realloc(l->lut, n * 3);
    if(!lut) return false;
    l->lut = lut;
    float hx = w * 0.5f, hy = h * 0.5f;
    float rr = LV_MIN((float)r, LV_MIN(hx, hy));
    float thick = b * 0.9f * l->strength / 100.0f;
    float lx = -0.7071f, ly = -0.7071f;                 /* light from the top left */
    size_t i = 0;
    BAND_FOREACH(w, h, rr0, b,
        float px = x_ + 0.5f - hx, py = y_ + 0.5f - hy;
        float qx = fabsf(px) - (hx - rr), qy = fabsf(py) - (hy - rr);
        float ox = qx > 0 ? qx : 0, oy = qy > 0 ? qy : 0;
        float len = sqrtf(ox * ox + oy * oy);
        float sdf = LV_MIN(LV_MAX(qx, qy), 0.0f) + len - rr;     /* < 0 inside */
        float d = -sdf;
        float sx = px < 0 ? -1.0f : 1.0f, sy = py < 0 ? -1.0f : 1.0f;
        float nx, ny;                                    /* outward normal */
        if(qx > 0 && qy > 0) { nx = sx * ox / len; ny = sy * oy / len; }
        else if(qx > qy) { nx = sx; ny = 0; }
        else { nx = 0; ny = sy; }
        float s = (d >= 0.0f && d < b) ? lens_shift_at(d / (float)b, thick) : 0.0f;   /* nothing outside the shape */
        float dx = -nx * s, dy = -ny * s;                /* sample from further inside */
        float lit = nx * lx + ny * ly;
        float spec = d < 0.0f ? 0.0f : (0.22f + (lit > 0 ? lit * lit : 0.45f * lit * lit)) * expf(-d / 1.6f);
        lut[i * 3 + 0] = (int8_t)LV_CLAMP(-127, (int)lroundf(dx), 127);
        lut[i * 3 + 1] = (int8_t)LV_CLAMP(-127, (int)lroundf(dy), 127);
        lut[i * 3 + 2] = (int8_t)LV_CLAMP(0, (int)(spec * 110.0f), 127);
        i++;
    )
    l->lw = w;
    l->lh = h;
    l->lr = r;
    return true;
}

static void lens_pre(lens_t * l, lv_draw_task_t * t, const lv_draw_blur_dsc_t * dsc, const lv_area_t * coords)
{
    lv_draw_buf_t * db = t->target_layer->draw_buf;
    if(!db) return;
    int32_t w = lv_area_get_width(coords), h = lv_area_get_height(coords);
    if(w < 8 || h < 8) return;
    if(!l->lut || l->lw != w || l->lh != h || l->lr != dsc->corner_radius) {
        if(!lut_build(l, w, h, dsc->corner_radius)) return;
    }
    /* Sharp copy of what is behind the pane (limited to what the layer buffer holds) */
    lv_area_t buf = t->target_layer->buf_area;
    if(!lv_area_intersect(&l->copy, coords, &buf)) {
        l->copy.x2 = l->copy.x1 - 1;
        return;
    }
    l->px = lv_color_format_get_size(db->header.cf);
    int32_t cw = lv_area_get_width(&l->copy), ch = lv_area_get_height(&l->copy);
    size_t need = (size_t)cw * ch * l->px;
    if(need > l->scratch_cap) {
        uint8_t * s = lv_realloc(l->scratch, need);
        if(!s) {
            l->copy.x2 = l->copy.x1 - 1;
            return;
        }
        l->scratch = s;
        l->scratch_cap = need;
    }
    for(int32_t y = 0; y < ch; y++) {
        const uint8_t * src = lv_draw_buf_goto_xy(db, l->copy.x1 - buf.x1, l->copy.y1 - buf.y1 + y);
        memcpy(l->scratch + (size_t)y * cw * l->px, src, (size_t)cw * l->px);
    }
}

static inline void px_get(const uint8_t * p, int32_t px, uint8_t * c)
{
    if(px >= 3) { c[0] = p[0]; c[1] = p[1]; c[2] = p[2]; }
    else {
        uint16_t v = (uint16_t)(p[0] | (p[1] << 8));    /* RGB565, little endian */
        c[0] = (uint8_t)((v & 0x1F) << 3);
        c[1] = (uint8_t)(((v >> 5) & 0x3F) << 2);
        c[2] = (uint8_t)(((v >> 11) & 0x1F) << 3);
    }
}

static inline void px_put(uint8_t * p, int32_t px, const uint8_t * c)
{
    if(px >= 3) { p[0] = c[0]; p[1] = c[1]; p[2] = c[2]; }
    else {
        uint16_t v = (uint16_t)((c[0] >> 3) | ((c[1] >> 2) << 5) | ((c[2] >> 3) << 11));
        p[0] = (uint8_t)(v & 0xFF);
        p[1] = (uint8_t)(v >> 8);
    }
}

static void lens_post(lens_t * l, lv_draw_task_t * t, const lv_area_t * coords)
{
    if(!l->lut || l->copy.x2 < l->copy.x1) return;
    lv_draw_buf_t * db = t->target_layer->draw_buf;
    lv_area_t buf = t->target_layer->buf_area;
    lv_area_t clip;
    if(!lv_area_intersect(&clip, &t->clip_area, &l->copy)) return;
    int32_t w = l->lw, h = l->lh, b = LV_MIN(l->bezel, LV_MIN(w, h) / 2);
    int32_t cw = lv_area_get_width(&l->copy), ch = lv_area_get_height(&l->copy);
    int32_t px = l->px;
    float rr0 = LV_MIN((float)l->lr, LV_MIN(w, h) * 0.5f);
    size_t i = 0;
    const int8_t * lut = l->lut;
    int32_t gx = coords->x1, gy = coords->y1;

    BAND_FOREACH(w, h, rr0, b,
        int32_t X = gx + x_, Y = gy + y_;
        if(X >= clip.x1 && X <= clip.x2 && Y >= clip.y1 && Y <= clip.y2) {
            int32_t dx = lut[i * 3 + 0], dy = lut[i * 3 + 1];
            int32_t spec = lut[i * 3 + 2];
            int32_t cx = X - l->copy.x1, cy = Y - l->copy.y1;
            uint8_t c[3];
            int32_t sx = LV_CLAMP(0, cx + dx, cw - 1), sy = LV_CLAMP(0, cy + dy, ch - 1);
            px_get(l->scratch + ((size_t)sy * cw + sx) * px, px, c);
            if(l->chroma && (dx | dy)) {
                uint8_t o[3];
                int32_t rx = LV_CLAMP(0, cx + (dx * 5) / 4, cw - 1), ry = LV_CLAMP(0, cy + (dy * 5) / 4, ch - 1);
                px_get(l->scratch + ((size_t)ry * cw + rx) * px, px, o);
                c[0] = o[0];
                rx = LV_CLAMP(0, cx + (dx * 3) / 4, cw - 1);
                ry = LV_CLAMP(0, cy + (dy * 3) / 4, ch - 1);
                px_get(l->scratch + ((size_t)ry * cw + rx) * px, px, o);
                c[2] = o[2];
            }
            if(spec) {
                c[0] = (uint8_t)(c[0] + ((255 - c[0]) * spec >> 7));
                c[1] = (uint8_t)(c[1] + ((255 - c[1]) * spec >> 7));
                c[2] = (uint8_t)(c[2] + ((255 - c[2]) * spec >> 7));
            }
            uint8_t * dst = lv_draw_buf_goto_xy(db, X - buf.x1, Y - buf.y1);
            px_put(dst, px, c);
        }
        i++;
    )
}

static void lens_hook(lv_draw_task_t * t, const lv_draw_blur_dsc_t * dsc, const lv_area_t * coords, bool post)
{
    if(!ui_lens_enabled()) return;
    lens_t * l = find(dsc->base.obj);
    if(!l) return;
    if(post) lens_post(l, t, coords);
    else lens_pre(l, t, dsc, coords);
}

static void lens_deleted_cb(lv_event_t * e)
{
    lens_t * l = find(lv_event_get_target_obj(e));
    if(!l) return;
    lv_free(l->lut);
    lv_free(l->scratch);
    lv_memzero(l, sizeof(*l));
}

void ui_lens(lv_obj_t * obj, int32_t bezel, int32_t strength, bool chroma)
{
    ui_glass(obj, 0xFFFFFF, LV_OPA_10, 10);             /* frosted interior, also guarantees a blur task */
    lens_t * l = find(obj);
    if(!l) {
        for(int i = 0; i < LENS_MAX && !l; i++) if(!s_lens[i].obj) l = &s_lens[i];
        if(!l) return;                                  /* table full: plain glass */
        lv_memzero(l, sizeof(*l));
        l->obj = obj;
        lv_obj_add_event_cb(obj, lens_deleted_cb, LV_EVENT_DELETE, NULL);
    }
    l->bezel = LV_MAX(4, bezel);
#ifdef S3_NATIVE_LAYOUT
    l->bezel = LV_MAX(1, s3_native_px(bezel));
#endif
    l->strength = LV_CLAMP(0, strength, 100);
    l->chroma = chroma;
    lv_free(l->lut);
    l->lut = NULL;                                      /* rebuilt on the next draw */
    lv_draw_sw_blur_hook = lens_hook;
    lv_obj_invalidate(obj);
}

#else /* OS_FX_LENS */

void ui_lens(lv_obj_t * obj, int32_t bezel, int32_t strength, bool chroma)
{
    LV_UNUSED(bezel);
    LV_UNUSED(strength);
    LV_UNUSED(chroma);
    ui_glass(obj, 0xFFFFFF, LV_OPA_20, 16);
}

#endif
