/**
 * @file control_center.c
 * iOS 18-style Control Center: pull down from the top-right edge over anything.
 *
 * Motion: the frosted backdrop fades in first, then the modules fly in row by row on springs
 * (scale + drop). While the finger drags, everything tracks it 1:1; pulling past fully open
 * rubber-bands and stretches the rows apart. Buttons squash on press and spring back.
 * Swipe up or tap empty space to close. Scale effects are skipped when OS_FX_HEAVY is 0 (ESP32).
 */
#include "sdk.h"
#include "glyph.h"

#define PULL_ZONE_H     26
#define PAD             16
#define GAP             12
#define TOP             56
#define OPEN_DIST       220.0f      /* finger travel for progress = 1 */
#define MOD_MAX         20

static struct {
    lv_obj_t * zone;
    lv_obj_t * sheet;
    lv_obj_t * mod[MOD_MAX];        /* modules that fly in */
    uint8_t row[MOD_MAX];
    int n;
    lv_obj_t * radio[4];            /* airplane, cellular, wifi, bluetooth */
    lv_obj_t * net;                 /* connectivity cluster tile */
    lv_obj_t * pop;                 /* expanded connectivity panel while open */
    lv_obj_t * pop_dim;
    lv_obj_t * pop_btn[6];
    lv_obj_t * pop_sub[6];
    lv_obj_t * focus, * torch;
    int32_t W, H, unit;
    int32_t y0;
    float p0, p, vy;
    float bg;                       /* backdrop blur/tint progress 0..1 */
    uint32_t last_tick;
    bool open, dragging;
} c;

static void build(void);
static void radios_refresh(void);
static void squish_cb(lv_event_t * e);
static void net_long_cb(lv_event_t * e);

/*---------------- Backdrop: blur radius grows with the finger instead of popping in ----------------*/

#define BLUR_MAX    36

static void bg_set(float v)
{
    c.bg = v;
    ui_glass_set(c.sheet, v, BLUR_MAX, LV_OPA_40);
}

static void bg_exec(void * o, int32_t v)
{
    LV_UNUSED(o);
    bg_set(v / 1000.0f);
}

static void bg_hidden_cb(lv_anim_t * a)
{
    LV_UNUSED(a);
    if(!c.open) lv_obj_set_hidden(c.sheet, true);   /* no blur cost while closed */
}

static void bg_to(float target, uint32_t delay)
{
    lv_anim_delete(c.sheet, bg_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, c.sheet);
    lv_anim_set_exec_cb(&a, bg_exec);
    lv_anim_set_values(&a, (int32_t)(c.bg * 1000.0f), (int32_t)(target * 1000.0f));
    lv_anim_set_duration(&a, 260);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, target > c.bg ? lv_anim_path_ease_out : lv_anim_path_ease_in_out);
    lv_anim_set_completed_cb(&a, bg_hidden_cb);
    lv_anim_start(&a);
}

/*---------------- Motion ----------------*/

static float rest_y(int row)
{
    return -(40.0f + row * 22.0f);
}

/* Interactive pose for progress p (0 closed .. 1 open, > 1 overscroll) */
static void pose(float p)
{
    c.p = p;
    lv_anim_delete(c.sheet, bg_exec);
    bg_set(LV_CLAMP(0.0f, p * 1.6f, 1.0f));                  /* blur follows the finger, a little ahead */
    float over = p > 1.0f ? (p - 1.0f) * OPEN_DIST : 0.0f;
    float band = over * 0.55f / (1.0f + over / 260.0f);      /* rubber band */

    for(int i = 0; i < c.n; i++) {
        int r = c.row[i];
        float q = LV_CLAMP(0.0f, (p - 0.18f - r * 0.06f) / 0.55f, 1.0f);
        fx_set(c.mod[i], FX_Y, (1.0f - q) * rest_y(r) + band * (0.25f + r * 0.28f));
        fx_set(c.mod[i], FX_OPA, q * 255.0f);
#if OS_FX_HEAVY
        fx_set(c.mod[i], FX_SCALE, 200.0f + 56.0f * q + band * 0.10f);
#endif
    }
}

static void settle(bool open)
{
    c.open = open;
    c.p = open ? 1.0f : 0.0f;
    float v = c.vy * 1000.0f;   /* finger px/s carried into the springs */
    for(int i = 0; i < c.n; i++) {
        int r = c.row[i];
        lv_obj_t * m = c.mod[i];
        if(open) {
            uint32_t d = (uint32_t)(r * 28);
            fx_to(m, FX_Y, 0, 0.46f, 0.66f, d, NULL);
            fx_kick(m, FX_Y, v * 0.3f);
            fx_to(m, FX_OPA, 255, 0.25f, 1.0f, d, NULL);
#if OS_FX_HEAVY
            fx_to(m, FX_SCALE, 256, 0.42f, 0.62f, d, NULL);
#endif
        }
        else {
            uint32_t d = (uint32_t)((4 - LV_MIN(r, 4)) * 12);
            fx_to(m, FX_Y, rest_y(r), 0.30f, 1.0f, d, NULL);
            fx_to(m, FX_OPA, 0, 0.20f, 1.0f, d, NULL);
#if OS_FX_HEAVY
            fx_to(m, FX_SCALE, 210, 0.30f, 1.0f, d, NULL);
#endif
        }
    }
    /* Backdrop: first in, last out */
    bg_to(open ? 1.0f : 0.0f, open ? 0 : 110);
    lv_obj_set_hidden(c.zone, open);
    lv_obj_set_clickable(c.sheet, true);
}

void cc_close(void)
{
    if(c.sheet && c.open) settle(false);
}

bool cc_visible(void)
{
    return c.sheet && !lv_obj_is_hidden(c.sheet) && c.bg > 0.15f;
}

/* Simulator test hook: show the sheet frozen at drag progress p (0..1.3) */
void cc_debug_pose(float p)
{
    if(!c.sheet) build();
    lv_obj_set_hidden(c.sheet, false);
    lv_obj_move_foreground(c.sheet);
    shell_statusbar_raise();
    pose(p);
}

void cc_debug_pop(void)
{
    cc_debug_pose(1.0f);
    net_long_cb(NULL);
}

void cc_set_enabled(bool en)
{
    if(c.zone) lv_obj_set_hidden(c.zone, !en || c.open);
}

/* Shared finger tracking for "pull down" (zone) and "push up" (sheet) */
static void track(lv_event_t * e, bool from_zone)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t pt;
    lv_indev_get_point(lv_indev_active(), &pt);

    if(code == LV_EVENT_PRESSED) {
        if(!c.sheet) build();
        radios_refresh();       /* Settings may have changed them */
        lv_obj_set_hidden(c.sheet, false);
        lv_obj_move_foreground(c.sheet);
        shell_statusbar_raise();
        c.y0 = pt.y;
        c.p0 = from_zone ? 0.0f : c.p;
        c.vy = 0;
        c.dragging = false;
        c.last_tick = lv_tick_get();
    }
    else if(code == LV_EVENT_PRESSING) {
        int32_t dy = pt.y - c.y0;
        if(!c.dragging && LV_ABS(dy) < 4) return;
        c.dragging = true;
        float p = c.p0 + dy / OPEN_DIST;
        if(p < 0.0f) p = 0.0f;
        uint32_t el = lv_tick_elaps(c.last_tick);
        if(el) {
            c.vy = 0.6f * c.vy + 0.4f * (p - c.p) * OPEN_DIST / (float)el;
            c.last_tick = lv_tick_get();
        }
        pose(p);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if(!c.dragging) {
            if(!from_zone) settle(false);           /* tap on empty space closes */
            else settle(false);
            return;
        }
        if(from_zone) settle(c.p > 0.40f || c.vy > 0.4f);
        else settle(!(c.p < 0.75f || c.vy < -0.4f));
    }
}

static void zone_cb(lv_event_t * e)
{
    track(e, true);
}

static void sheet_cb(lv_event_t * e)
{
    if(lv_event_get_target_obj(e) != c.sheet) return;
    track(e, false);
}

/*---------------- Modules ----------------*/

static void squash_cb(lv_event_t * e)
{
#if OS_FX_HEAVY
    lv_obj_t * o = lv_event_get_current_target_obj(e);
    if(lv_event_get_code(e) == LV_EVENT_PRESSED) fx_to(o, FX_SCALE, 232, 0.18f, 1.0f, 0, NULL);
    else fx_to(o, FX_SCALE, 256, 0.40f, 0.45f, 0, NULL);
#else
    LV_UNUSED(e);
#endif
}

static void springy(lv_obj_t * o)
{
    lv_obj_add_event_cb(o, squash_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(o, squash_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(o, squash_cb, LV_EVENT_PRESS_LOST, NULL);
}

static lv_obj_t * module(lv_obj_t * o, int row)
{
    if(c.n < MOD_MAX) {
        c.mod[c.n] = o;
        c.row[c.n] = (uint8_t)row;
        c.n++;
    }
    return o;
}

static lv_obj_t * tile(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t * t = ui_box(parent);
    lv_obj_set_pos(t, PAD + x * (c.unit + GAP), TOP + y * (c.unit + GAP));
    lv_obj_set_size(t, w * c.unit + (w - 1) * GAP, h * c.unit + (h - 1) * GAP);
    lv_obj_set_style_radius(t, 22, 0);
    lv_obj_set_style_bg_color(t, lv_color_hex(0x3A3A3C), 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_50, 0);
    return module(t, y);
}

static void paint_toggle(lv_obj_t * b, bool on, uint32_t on_color)
{
    lv_obj_set_style_bg_color(b, lv_color_hex(on ? on_color : 0x48484A), 0);
    lv_obj_set_style_bg_opa(b, on ? LV_OPA_COVER : LV_OPA_70, 0);
    lv_obj_set_style_text_color(lv_obj_get_child(b, 0), lv_color_hex(on && on_color == 0xFFFFFF ? 0x000000 : 0xFFFFFF), 0);
}

static void radios_refresh(void)
{
    paint_toggle(c.radio[0], os_settings.airplane, UI_ORANGE);
    paint_toggle(c.radio[1], os_settings.cellular && !os_settings.airplane, UI_GREEN);
    paint_toggle(c.radio[2], os_settings.wifi, UI_BLUE);
    paint_toggle(c.radio[3], os_settings.bluetooth, UI_BLUE);
    paint_toggle(c.focus, os_settings.dnd, 0x5E5CE6);
    paint_toggle(c.torch, os_settings.flashlight, 0xFFFFFF);
}

static void radio_cb(lv_event_t * e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if(i == 0) {
        os_settings.airplane = !os_settings.airplane;
        if(os_settings.airplane) os_settings.wifi = os_settings.bluetooth = false;
    }
    else if(i == 1) os_settings.cellular = !os_settings.cellular;
    else if(i == 2) os_settings.wifi = !os_settings.wifi;
    else if(i == 3) os_settings.bluetooth = !os_settings.bluetooth;
    else if(i == 4) os_settings.dnd = !os_settings.dnd;
    else if(i == 5) os_settings.flashlight = !os_settings.flashlight;
    radios_refresh();
    shell_settings_apply();
    os_save();
}

static lv_obj_t * round_toggle(lv_obj_t * parent, const char * sym, int idx, int32_t size)
{
    lv_obj_t * b = ui_box(parent);
    lv_obj_set_size(b, size, size);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_clickable(b, true);
    springy(b);
    lv_obj_add_event_cb(b, radio_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
    lv_obj_center(ui_label(b, sym, &lv_font_montserrat_20, 0xFFFFFF));
    return b;
}

static void slider_cb(lv_event_t * e)
{
    uint8_t * v = lv_event_get_user_data(e);
    *v = (uint8_t)lv_slider_get_value(lv_event_get_target_obj(e));
    if(v == &os_settings.brightness && *v < 10) *v = 10;
    shell_settings_apply();
    if(lv_event_get_code(e) == LV_EVENT_RELEASED) os_save();
}

static void pill_slider(lv_obj_t * parent, int32_t x, glyph_t g, uint8_t * value)
{
    lv_obj_t * s = lv_slider_create(parent);
    lv_obj_set_pos(s, PAD + x * (c.unit + GAP), TOP);
    lv_obj_set_size(s, c.unit, 2 * c.unit + GAP);
    lv_slider_set_range(s, 0, 100);
    lv_slider_set_value(s, *value, LV_ANIM_OFF);
    lv_obj_set_style_radius(s, 22, 0);
    lv_obj_set_style_radius(s, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x3A3A3C), 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_60, 0);
    lv_obj_set_style_bg_color(s, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_clip_corner(s, true, 0);
    lv_obj_set_style_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(s, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(s, slider_cb, LV_EVENT_VALUE_CHANGED, value);
    lv_obj_add_event_cb(s, slider_cb, LV_EVENT_RELEASED, value);
    lv_obj_add_event_cb(s, squish_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s, squish_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s, squish_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s, squish_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_t * l = glyph_create(s, g, 26, 0x8E8E93);
    lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, -12);
    module(s, 0);
}

/*---------------- Squishy sliders: pull past the end and the pill stretches like rubber ----------------*/

static float rubber(float x)
{
    return x * 0.5f / (1.0f + x / 140.0f);
}

#define SQUISH_PULSE    6.0f        /* px the pill swells by on touch */
#define SQUISH_MAX_H    22.0f       /* cap on the overdrag stretch */
#define SQUISH_MAX_W    10.0f

static void squish_cb(lv_event_t * e)
{
#if OS_FX_HEAVY
    lv_obj_t * s = lv_event_get_target_obj(e);
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        /* touch: the pill swells a little; it only stretches further when the finger drags past an end */
        lv_point_t p;
        lv_indev_get_point(lv_indev_active(), &p);
        lv_area_t a;
        lv_obj_get_coords(s, &a);
        a.x1 -= (int32_t)fx_get(s, FX_X); a.x2 -= (int32_t)fx_get(s, FX_X);
        a.y1 -= (int32_t)fx_get(s, FX_Y); a.y2 -= (int32_t)fx_get(s, FX_Y);
        float oy = p.y < a.y1 ? (float)(a.y1 - p.y) : p.y > a.y2 ? (float)(p.y - a.y2) : 0.0f;
        float ox = p.x < a.x1 ? (float)(a.x1 - p.x) : p.x > a.x2 ? (float)(p.x - a.x2) : 0.0f;
        float dh = LV_MIN(rubber(oy), SQUISH_MAX_H), dw = LV_MIN(rubber(ox), SQUISH_MAX_W);
        float up = p.y < a.y1 ? -1.0f : 1.0f;
        float sx = p.x < a.x1 ? -1.0f : 1.0f;
        float resp = code == LV_EVENT_PRESSED ? 0.28f : 0.10f, damp = code == LV_EVENT_PRESSED ? 0.55f : 1.0f;
        fx_to(s, FX_DH, SQUISH_PULSE + dh, resp, damp, 0, NULL);
        fx_to(s, FX_DW, SQUISH_PULSE + dw - dh * 0.28f, resp, damp, 0, NULL);   /* longer = a little thinner */
        fx_to(s, FX_Y, up * dh * 0.5f, resp, damp, 0, NULL);
        fx_to(s, FX_X, sx * dw * 0.5f, resp, damp, 0, NULL);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        fx_to(s, FX_DH, 0, 0.42f, 0.45f, 0, NULL);          /* underdamped: wobbles back */
        fx_to(s, FX_DW, 0, 0.42f, 0.45f, 0, NULL);
        fx_to(s, FX_Y, 0, 0.42f, 0.45f, 0, NULL);
        fx_to(s, FX_X, 0, 0.42f, 0.45f, 0, NULL);
    }
#else
    LV_UNUSED(e);
#endif
}

/*---------------- Connectivity panel: hold the cluster and it morphs open ----------------*/

static const char * const k_pop_name[6] = { "Airplane", "Cellular", "Wi-Fi", "Bluetooth", "AirDrop", "Hotspot" };
static const char * const k_pop_sym[6] = { LV_SYMBOL_SHUFFLE, "LTE", LV_SYMBOL_WIFI, LV_SYMBOL_BLUETOOTH,
                                           LV_SYMBOL_UPLOAD, LV_SYMBOL_LOOP };
static const uint32_t k_pop_col[6] = { 0xFF9F0A, 0x30D158, 0x0A84FF, 0x0A84FF, 0x0A84FF, 0x30D158 };

static bool pop_state(int i)
{
    switch(i) {
        case 0: return os_settings.airplane;
        case 1: return os_settings.cellular && !os_settings.airplane;
        case 2: return os_settings.wifi;
        case 3: return os_settings.bluetooth;
        case 4: return sdk_get_int("cc.airdrop", 0) != 0;
        default: return sdk_get_int("cc.hotspot", 0) != 0;
    }
}

static void pop_refresh(void)
{
    for(int i = 0; i < 6; i++) {
        if(!c.pop_btn[i]) continue;
        bool on = pop_state(i);
        lv_obj_set_style_bg_color(c.pop_btn[i], lv_color_hex(on ? k_pop_col[i] : 0x48484A), 0);
        lv_label_set_text(c.pop_sub[i], on ? "On" : "Off");
    }
}

static void pop_gone_cb(lv_obj_t * o)
{
    LV_UNUSED(o);
    if(c.pop_dim) lv_obj_delete(c.pop_dim);
    c.pop = c.pop_dim = NULL;
    lv_memzero(c.pop_btn, sizeof(c.pop_btn));
}

static void pop_close(void)
{
    if(!c.pop || !c.pop_dim) return;
    lv_obj_set_clickable(c.pop_dim, false);
    lv_area_t a, pa;
    lv_obj_get_coords(c.net, &a);
    lv_obj_get_coords(c.pop, &pa);
    float tx = (a.x1 + a.x2) * 0.5f - (pa.x1 + pa.x2) * 0.5f + fx_get(c.pop, FX_X);
    float ty = (a.y1 + a.y2) * 0.5f - (pa.y1 + pa.y2) * 0.5f + fx_get(c.pop, FX_Y);
    fx_to(c.net, FX_OPA, 255, 0.2f, 1.0f, 60, NULL);
    fx_to(c.pop_dim, FX_OPA, 0, 0.22f, 1.0f, 0, NULL);
#if OS_FX_HEAVY
    fx_to(c.pop, FX_SCALE, 256.0f * lv_obj_get_width(c.net) / lv_obj_get_width(c.pop), 0.30f, 1.0f, 0, NULL);
#endif
    fx_to(c.pop, FX_X, tx, 0.30f, 1.0f, 0, NULL);
    fx_to(c.pop, FX_Y, ty, 0.30f, 1.0f, 0, NULL);
    fx_to(c.pop, FX_OPA, 0, 0.18f, 1.0f, 90, pop_gone_cb);
}

static void pop_btn_cb(lv_event_t * e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if(i < 4) {
        radio_cb(e);                    /* same switches as the cluster */
    }
    else {
        const char * key = i == 4 ? "cc.airdrop" : "cc.hotspot";
        sdk_set_int(key, !sdk_get_int(key, 0));
    }
    pop_refresh();
    radios_refresh();
#if OS_FX_HEAVY
    fx_set(c.pop_btn[i], FX_SCALE, 300);
    fx_to(c.pop_btn[i], FX_SCALE, 256, 0.40f, 0.50f, 0, NULL);
#endif
}

static void pop_dim_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    pop_close();
}

static void net_long_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(c.pop) return;
    c.pop_dim = ui_box(c.sheet);
    lv_obj_set_size(c.pop_dim, c.W, c.H);
    lv_obj_set_clickable(c.pop_dim, true);
    lv_obj_set_style_bg_color(c.pop_dim, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(c.pop_dim, LV_OPA_30, 0);
    lv_obj_add_event_cb(c.pop_dim, pop_dim_cb, LV_EVENT_CLICKED, NULL);

    int32_t pw = c.W - 2 * PAD - 20, ph = 3 * 68 + 28;
    c.pop = ui_box(c.pop_dim);
    lv_obj_set_size(c.pop, pw, ph);
    lv_obj_set_pos(c.pop, (c.W - pw) / 2, TOP - 6);
    lv_obj_set_clickable(c.pop, true);                  /* taps inside do not dismiss */
    lv_obj_set_style_radius(c.pop, 34, 0);
    ui_glass(c.pop, 0x3A3A3C, LV_OPA_60, 30);
    lv_obj_set_style_pad_all(c.pop, 14, 0);
    lv_obj_set_style_pad_row(c.pop, 12, 0);
    lv_obj_set_style_pad_column(c.pop, 8, 0);
    lv_obj_set_flex_flow(c.pop, LV_FLEX_FLOW_ROW_WRAP);
    for(int i = 0; i < 6; i++) {
        lv_obj_t * cell = ui_box(c.pop);
        lv_obj_set_size(cell, (pw - 28 - 8) / 2, 56);
        lv_obj_t * b = ui_box(cell);
        lv_obj_set_size(b, 42, 42);
        lv_obj_align(b, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_clickable(b, true);
        lv_obj_add_event_cb(b, pop_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_center(ui_label(b, k_pop_sym[i], i == 1 ? &lv_font_montserrat_14 : &lv_font_montserrat_20, 0xFFFFFF));
        c.pop_btn[i] = b;
        lv_obj_t * t = ui_label(cell, k_pop_name[i], &lv_font_montserrat_12, 0xFFFFFF);
        lv_obj_align(t, LV_ALIGN_LEFT_MID, 48, -8);
        c.pop_sub[i] = ui_label(cell, "", &lv_font_montserrat_12, 0xC7C7CC);
        lv_obj_align(c.pop_sub[i], LV_ALIGN_LEFT_MID, 48, 8);
    }
    pop_refresh();
    lv_obj_update_layout(c.pop_dim);

    /* Start as the 2x2 tile (same centre, same width) and spring out to full size */
    lv_area_t a, pa;
    lv_obj_get_coords(c.net, &a);
    lv_obj_get_coords(c.pop, &pa);
    float tx = (a.x1 + a.x2) * 0.5f - (pa.x1 + pa.x2) * 0.5f;
    float ty = (a.y1 + a.y2) * 0.5f - (pa.y1 + pa.y2) * 0.5f;
    fx_set(c.pop, FX_X, tx);
    fx_set(c.pop, FX_Y, ty);
    fx_set(c.pop_dim, FX_OPA, 0);
    fx_set(c.pop, FX_OPA, 120);
    fx_to(c.pop, FX_X, 0, 0.50f, 0.64f, 0, NULL);
    fx_to(c.pop, FX_Y, 0, 0.50f, 0.64f, 0, NULL);
    fx_to(c.pop, FX_OPA, 255, 0.22f, 1.0f, 0, NULL);
    fx_to(c.pop_dim, FX_OPA, 255, 0.22f, 1.0f, 0, NULL);
#if OS_FX_HEAVY
    fx_set(c.pop, FX_SCALE, 256.0f * lv_obj_get_width(c.net) / pw);
    fx_to(c.pop, FX_SCALE, 256, 0.50f, 0.64f, 0, NULL);
#endif
    fx_to(c.net, FX_OPA, 0, 0.12f, 1.0f, 0, NULL);
}

static void open_app_cb(lv_event_t * e)
{
    lv_area_t a;
    lv_obj_get_coords(lv_event_get_current_target_obj(e), &a);
    settle(false);
    int id = sdk_app_index(lv_event_get_user_data(e));
    if(id >= 0) shell_open_app_from(id, &a);     /* exits with a fade, not into an icon */
}

static void edit_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    settle(false);
    if(!lock_visible()) home_edit_enter();
}

static void lock_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    settle(false);
    lock_now();
}

static void shortcut(lv_obj_t * parent, int32_t x, int32_t y, const char * sym, lv_event_cb_t cb, const void * ud)
{
    lv_obj_t * t = tile(parent, x, y, 1, 1);
    lv_obj_set_style_radius(t, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_clickable(t, true);
    lv_obj_set_style_bg_opa(t, LV_OPA_90, LV_STATE_PRESSED);
    springy(t);
    lv_obj_add_event_cb(t, cb, LV_EVENT_CLICKED, (void *)ud);
    lv_obj_center(ui_label(t, sym, &lv_font_montserrat_22, 0xFFFFFF));
}

extern const app_desc_t app_clock, app_calculator, app_photos, app_settings, app_notify;

static void build(void)
{
    c.sheet = ui_box(lv_layer_top());
    lv_obj_set_size(c.sheet, c.W, c.H);
    ui_glass(c.sheet, 0x101014, LV_OPA_40, 36);
    lv_obj_set_press_lock(c.sheet, true);
    lv_obj_add_event_cb(c.sheet, sheet_cb, LV_EVENT_ALL, NULL);

    /* Connectivity cluster (2x2) */
    lv_obj_t * net = tile(c.sheet, 0, 0, 2, 2);
    c.net = net;
    lv_obj_set_clickable(net, true);
    lv_obj_add_event_cb(net, net_long_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_set_flex_flow(net, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(net, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_pad_all(net, 10, 0);
    static const char * const syms[4] = { LV_SYMBOL_SHUFFLE, "LTE", LV_SYMBOL_WIFI, LV_SYMBOL_BLUETOOTH };
    for(int i = 0; i < 4; i++) {
        c.radio[i] = round_toggle(net, syms[i], i, c.unit - 14);
        lv_obj_set_event_bubble(c.radio[i], true);
    }
    lv_obj_set_style_text_font(lv_obj_get_child(c.radio[1], 0), &lv_font_montserrat_12, 0);

    /* Brightness + volume pills */
    pill_slider(c.sheet, 2, GLYPH_SUN, &os_settings.brightness);
    pill_slider(c.sheet, 3, GLYPH_SPEAKER, &os_settings.volume);

    /* Focus (2x1) */
    lv_obj_t * f = tile(c.sheet, 0, 2, 2, 1);
    lv_obj_set_flex_flow(f, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(f, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(f, 10, 0);
    lv_obj_set_style_pad_column(f, 10, 0);
    c.focus = round_toggle(f, LV_SYMBOL_BELL, 4, 40);
    ui_label(f, "Focus", &lv_font_montserrat_16, 0xFFFFFF);

    c.torch = round_toggle(c.sheet, LV_SYMBOL_CHARGE, 5, c.unit);
    lv_obj_set_pos(c.torch, PAD + 2 * (c.unit + GAP), TOP + 2 * (c.unit + GAP));
    module(c.torch, 2);
    shortcut(c.sheet, 3, 2, LV_SYMBOL_LOOP, open_app_cb, &app_clock);
    shortcut(c.sheet, 0, 3, LV_SYMBOL_PLUS, open_app_cb, &app_calculator);
    shortcut(c.sheet, 1, 3, LV_SYMBOL_IMAGE, open_app_cb, &app_photos);
    shortcut(c.sheet, 2, 3, LV_SYMBOL_SETTINGS, open_app_cb, &app_settings);
    shortcut(c.sheet, 3, 3, LV_SYMBOL_EDIT, edit_cb, NULL);
    shortcut(c.sheet, 0, 4, LV_SYMBOL_EYE_CLOSE, lock_cb, NULL);
    shortcut(c.sheet, 1, 4, LV_SYMBOL_BELL, open_app_cb, &app_notify);

    lv_obj_t * hint = ui_label(c.sheet, "Swipe up to close", &lv_font_montserrat_12, 0x8E8E93);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -26);
    module(hint, 5);
    radios_refresh();
    pose(0);
    lv_obj_set_hidden(c.sheet, true);
}

void cc_init(void)
{
    c.W = lv_display_get_horizontal_resolution(lv_display_get_default());
    c.H = lv_display_get_vertical_resolution(lv_display_get_default());
    c.unit = (c.W - 2 * PAD - 3 * GAP) / 4;

    c.zone = ui_box(lv_layer_top());
    lv_obj_set_size(c.zone, c.W / 2, PULL_ZONE_H);
    lv_obj_align(c.zone, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_clickable(c.zone, true);
    lv_obj_set_press_lock(c.zone, true);
    lv_obj_set_gesture_bubble(c.zone, false);
    lv_obj_add_event_cb(c.zone, zone_cb, LV_EVENT_ALL, NULL);
}

void cc_command_open(void)
{
 if(!c.sheet) build();
 if(c.open) return;
 radios_refresh();
 c.vy = 0;
 lv_obj_set_hidden(c.sheet, false);
 lv_obj_move_foreground(c.sheet);
 shell_statusbar_raise();
 pose(0);
 settle(true);
}

bool s3_cc_busy(void)
{
 if(c.sheet && lv_anim_get(c.sheet, bg_exec)) return true;
 for(int i = 0; i < c.n; i++)
  if(fx_busy(c.mod[i], FX_Y) || fx_busy(c.mod[i], FX_OPA) || fx_busy(c.mod[i], FX_SCALE)) return true;
 return false;
}
