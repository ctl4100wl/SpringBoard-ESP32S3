/**
 * @file glass_text.c
 * Liquid-glass lettering: the glyphs themselves are the lens.
 *
 * Done once per text change (the Lock Screen clock changes once a minute), so it is cheap enough for an ESP32-S3:
 *  1. The text is rendered white into an A8 mask (slightly fattened so thin strokes have room for a bezel).
 *  2. A chamfer distance transform gives, for every pixel, the distance to the glyph edge and an inward normal.
 *  3. Each pixel inside a glyph copies the wallpaper from the displaced position (Snell's law bezel profile from
 *     lens.c), so the stroke edges bend the picture like a glass rod, then gets a faint tint, a specular rim lit
 *     from the top left and a soft shadow underneath.
 * The result is a normal ARGB8888 image.
 */
#include <math.h>
#include <string.h>
#include "sdk.h"

/* Refraction shift (px) at distance u in 0..1 from the rim (0 = rim). Convex squircle surface, a ray entering
 * straight down bends by Snell's law (glass index 1.5) and leaves after `thick` px of glass. Shared with lens.c. */
float lens_shift_at(float u, float thick)
{
    u = LV_CLAMP(0.002f, u, 1.0f);
    float v = 1.0f - u;
    float v4 = v * v * v * v;
    float slope = (v * v * v) / powf(LV_MAX(1.0f - v4, 1e-4f), 0.75f);   /* dh/du of (1-(1-u)^4)^(1/4) */
    float t1 = atanf(slope);
    float t2 = asinf(sinf(t1) / 1.5f);
    return LV_MIN(thick * tanf(t1 - t2), thick * 1.4f);
}

#define INF 0x3FFF

/* Two pass 3-4 chamfer distance transform of a binary mask (distance in 1/3 px units to the nearest "set" pixel) */
static void chamfer(const uint8_t * set, int32_t w, int32_t h, uint16_t * d)
{
    for(int32_t i = 0; i < w * h; i++) d[i] = set[i] ? 0 : INF;
    for(int32_t y = 0; y < h; y++) {
        for(int32_t x = 0; x < w; x++) {
            uint16_t v = d[y * w + x];
            if(x > 0) v = LV_MIN(v, d[y * w + x - 1] + 3);
            if(y > 0) {
                v = LV_MIN(v, d[(y - 1) * w + x] + 3);
                if(x > 0) v = LV_MIN(v, d[(y - 1) * w + x - 1] + 4);
                if(x < w - 1) v = LV_MIN(v, d[(y - 1) * w + x + 1] + 4);
            }
            d[y * w + x] = v;
        }
    }
    for(int32_t y = h - 1; y >= 0; y--) {
        for(int32_t x = w - 1; x >= 0; x--) {
            uint16_t v = d[y * w + x];
            if(x < w - 1) v = LV_MIN(v, d[y * w + x + 1] + 3);
            if(y < h - 1) {
                v = LV_MIN(v, d[(y + 1) * w + x] + 3);
                if(x < w - 1) v = LV_MIN(v, d[(y + 1) * w + x + 1] + 4);
                if(x > 0) v = LV_MIN(v, d[(y + 1) * w + x - 1] + 4);
            }
            d[y * w + x] = v;
        }
    }
}

static int s_mode;     /* 0 = clear liquid glass, 1 = frosted, 2 = prism (strong colour fringing) */

void ui_glass_text_mode(int mode)
{
    s_mode = mode;
}

/* Separable box blur (two passes ~ gaussian) of an RGB crop, used for the frosted body */
static void box_blur(uint8_t * px, int32_t w, int32_t h, int32_t r)
{
    uint8_t * tmp = lv_malloc((size_t)w * h * 3);
    if(!tmp) return;
    for(int pass = 0; pass < 2; pass++) {
        for(int32_t y = 0; y < h; y++) {
            for(int c = 0; c < 3; c++) {
                int32_t acc = 0, n = 0;
                for(int32_t x = -r; x <= r; x++) if(x >= 0 && x < w) { acc += px[(y * w + x) * 3 + c]; n++; }
                for(int32_t x = 0; x < w; x++) {
                    tmp[(y * w + x) * 3 + c] = (uint8_t)(acc / n);
                    int32_t xo = x - r, xi = x + r + 1;
                    if(xo >= 0) { acc -= px[(y * w + xo) * 3 + c]; n--; }
                    if(xi < w) { acc += px[(y * w + xi) * 3 + c]; n++; }
                }
            }
        }
        for(int32_t x = 0; x < w; x++) {
            for(int c = 0; c < 3; c++) {
                int32_t acc = 0, n = 0;
                for(int32_t y = -r; y <= r; y++) if(y >= 0 && y < h) { acc += tmp[(y * w + x) * 3 + c]; n++; }
                for(int32_t y = 0; y < h; y++) {
                    px[(y * w + x) * 3 + c] = (uint8_t)(acc / n);
                    int32_t yo = y - r, yi = y + r + 1;
                    if(yo >= 0) { acc -= tmp[(yo * w + x) * 3 + c]; n--; }
                    if(yi < h) { acc += tmp[(yi * w + x) * 3 + c]; n++; }
                }
            }
        }
    }
    lv_free(tmp);
}

lv_draw_buf_t * ui_glass_text(lv_obj_t * bg, const char * text, const lv_font_t * font, int32_t pad, int32_t ox,
                              int32_t oy, uint32_t tint, int32_t bezel, int32_t strength, bool chroma)
{
    /* 1. mask */
    lv_obj_t * box = ui_box(lv_layer_sys());
    ui_label(box, text, font, 0xFFFFFF);
    lv_obj_set_style_pad_all(box, pad, 0);
    lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(box, -2000, 0);
    lv_obj_update_layout(box);
    lv_draw_buf_t * mask = lv_snapshot_take(box, LV_COLOR_FORMAT_A8);
    lv_obj_delete(box);
    if(!mask) return NULL;
#ifdef S3_NATIVE_LAYOUT
    ox = s3_native_px(ox);
    oy = s3_native_px(oy);
    bezel = LV_MAX(1, s3_native_px(bezel));
#endif
    int32_t w = (int32_t)mask->header.w, h = (int32_t)mask->header.h, ms = (int32_t)mask->header.stride;

    /* 2. wallpaper behind the text (everything in `bg` below the clock must already be what we want to bend) */
    lv_draw_buf_t * wall = lv_snapshot_take(bg, LV_COLOR_FORMAT_ARGB8888);
    if(!wall) {
        lv_draw_buf_destroy(mask);
        return NULL;
    }
    int32_t ws = (int32_t)wall->header.stride, ww = (int32_t)wall->header.w, wh = (int32_t)wall->header.h;

    lv_draw_buf_t * out = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_ARGB8888, 0);
    uint8_t * set_in = lv_malloc((size_t)w * h);
    uint8_t * set_out = lv_malloc((size_t)w * h);
    uint16_t * din = lv_malloc((size_t)w * h * 2);
    uint16_t * dout = lv_malloc((size_t)w * h * 2);
    /* frosted: a blurred copy of the wallpaper under the text (with a margin for refraction) */
    const int32_t fm = 24;
    int32_t fw = w + 2 * fm, fh = h + 2 * fm;
    uint8_t * frost = NULL;
    if(s_mode == 1) {
        frost = lv_malloc((size_t)fw * fh * 3);
        if(frost) {
            for(int32_t y = 0; y < fh; y++)
                for(int32_t x = 0; x < fw; x++) {
                    int32_t sx = LV_CLAMP(0, ox + x - fm, ww - 1), sy = LV_CLAMP(0, oy + y - fm, wh - 1);
                    const uint8_t * wp = wall->data + (size_t)sy * ws + (size_t)sx * 4;
                    uint8_t * f = frost + (y * fw + x) * 3;
                    f[0] = wp[2]; f[1] = wp[1]; f[2] = wp[0];
                }
            box_blur(frost, fw, fh, 5);
        }
    }
    if(!out || !set_in || !set_out || !din || !dout) goto done;

    /* fatten by 1.5 px so the bezel has room: pixel is "glass" when within 1.5 px of coverage */
    for(int32_t y = 0; y < h; y++)
        for(int32_t x = 0; x < w; x++) set_in[y * w + x] = mask->data[y * ms + x] > 127;
    chamfer(set_in, w, h, dout);                         /* distance to nearest glyph pixel */
    for(int32_t i = 0; i < w * h; i++) set_in[i] = dout[i] <= 9;       /* <= 3 px: strokes need room for a bezel */
    for(int32_t i = 0; i < w * h; i++) set_out[i] = !set_in[i];
    chamfer(set_out, w, h, din);                         /* distance to nearest non-glass pixel = depth inside */
    chamfer(set_in, w, h, dout);                         /* distance outside the glass */

    float thick = bezel * 0.9f * strength / 100.0f;
    for(int32_t y = 0; y < h; y++) {
        uint8_t * o = out->data + (size_t)y * out->header.stride;
        for(int32_t x = 0; x < w; x++, o += 4) {
            int32_t i = y * w + x;
            o[0] = o[1] = o[2] = o[3] = 0;
            float d_in = din[i] / 3.0f;                  /* inside depth, 0 at the rim */
            float d_out = dout[i] / 3.0f;
            if(!set_in[i]) {
                /* soft contact shadow below-right of the glass so it sits on the wallpaper */
                int32_t sx = x - 2, sy = y - 3;
                float ds = (sx >= 0 && sy >= 0) ? dout[sy * w + sx] / 3.0f : 99.0f;
                float sh = ds < 8.0f ? 0.30f * expf(-ds / 2.2f) : 0.0f;
                if(d_out <= 1.0f) sh = LV_MAX(sh, 0.0f);
                if(d_out > 1.0f) {
                    o[3] = (uint8_t)(255.0f * sh);
                    continue;
                }
            }
            /* soft coverage at the boundary */
            float cover = set_in[i] ? 1.0f : LV_MAX(0.0f, 1.0f - d_out);
            if(set_in[i] && d_in < 1.0f) cover = 0.65f + 0.35f * d_in;
            /* inward normal from the depth field */
            float gx = ((x < w - 1 ? din[i + 1] : din[i]) - (x > 0 ? din[i - 1] : din[i])) / 6.0f;
            float gy = ((y < h - 1 ? din[i + w] : din[i]) - (y > 0 ? din[i - w] : din[i])) / 6.0f;
            float gl = sqrtf(gx * gx + gy * gy);
            float nx = gl > 0.01f ? gx / gl : 0.0f, ny = gl > 0.01f ? gy / gl : 0.0f;     /* points inside */
            float s = set_in[i] && d_in < bezel ? lens_shift_at(d_in / (float)bezel, thick) : 0.0f;
            /* diffraction: red, green and blue bend by slightly different amounts (same fringe as the Glass lab) */
            float r, g, b;
            {
                static const float k_ch[3] = { 1.32f, 1.0f, 0.68f };       /* R, G, B shift factors */
                static const float k_prism[3] = { 1.9f, 1.0f, 0.15f };
                float out_c[3];
                for(int ch = 0; ch < 3; ch++) {
                    float shift = s_mode == 2 ? k_prism[ch] : (chroma ? k_ch[ch] : 1.0f);
                    float ss = s_mode == 2 ? s + 2.5f * (set_in[i] && d_in < bezel ? 1.0f : 0.0f) : s;
                    int32_t dx = (int32_t)lroundf(nx * ss * shift), dy = (int32_t)lroundf(ny * ss * shift);
                    if(frost) {
                        int32_t fx = LV_CLAMP(0, x + fm + dx, fw - 1), fy = LV_CLAMP(0, y + fm + dy, fh - 1);
                        out_c[ch] = frost[(fy * fw + fx) * 3 + ch];
                        continue;
                    }
                    int32_t sx = LV_CLAMP(0, ox + x + dx, ww - 1);
                    int32_t sy = LV_CLAMP(0, oy + y + dy, wh - 1);
                    const uint8_t * wp = wall->data + (size_t)sy * ws + (size_t)sx * 4;
                    out_c[ch] = wp[2 - ch];                                 /* ARGB8888 is B,G,R,A in memory */
                }
                r = out_c[0]; g = out_c[1]; b = out_c[2];
            }
            /* glass body: slight frost + tint + vertical sheen */
            float t = s_mode == 1 ? 0.30f : 0.16f;
            if(s_mode == 1) { r += (255.0f - r) * 0.22f; g += (255.0f - g) * 0.22f; b += (255.0f - b) * 0.22f; }
            /* inner shade on the far side of the bezel gives the stroke volume */
            float shade = set_in[i] ? 0.18f * LV_MAX(0.0f, nx * -0.7071f + ny * -0.7071f) * expf(-d_in / 3.0f) : 0.0f;
            r *= 1.0f - shade; g *= 1.0f - shade; b *= 1.0f - shade;
            float tr = (tint >> 16) & 0xFF, tg = (tint >> 8) & 0xFF, tb = tint & 0xFF;
            float sheen = 14.0f * (1.0f - (float)y / (float)h);
            r = r * (1.0f - t) + tr * t + sheen;
            g = g * (1.0f - t) + tg * t + sheen;
            b = b * (1.0f - t) + tb * t + sheen;
            /* specular rim lit from the top left (normal points inside, so the lit side faces -n) */
            float lit = -(nx * -0.7071f + ny * -0.7071f);
            float rim = (0.20f + (lit > 0 ? lit * lit : 0.35f * lit * lit)) * expf(-d_in / 1.7f);
            if(!set_in[i]) rim = 0.0f;
            r += (255.0f - r) * rim; g += (255.0f - g) * rim; b += (255.0f - b) * rim;
            o[0] = (uint8_t)LV_CLAMP(0, (int)b, 255);
            o[1] = (uint8_t)LV_CLAMP(0, (int)g, 255);
            o[2] = (uint8_t)LV_CLAMP(0, (int)r, 255);
            o[3] = (uint8_t)(255.0f * cover * 0.97f);
        }
    }
done:
    lv_free(frost);
    lv_free(set_in);
    lv_free(set_out);
    lv_free(din);
    lv_free(dout);
    lv_draw_buf_destroy(mask);
    lv_draw_buf_destroy(wall);
    return out;
}
