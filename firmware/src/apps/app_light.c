/**
 * @file app_light.c
 * Paper: a light-themed test app. Its scrolling page is made of bands in different colours; whichever band is
 * under the status bar decides whether the clock and icons turn dark (bright band) or stay white (dark band),
 * easing between the two as you scroll. A switch flips the whole page between light and dark.
 */
#include "../os/sdk.h"

typedef struct {
    uint32_t bg;
    uint32_t fg;
    const char * title;
    const char * body;
} band_t;

static const band_t k_bands[] = {
    { 0xFFFFFF, 0x111111, "Paper", "A quieter place to begin." },
    { 0xFFD60A, 0x1C1C1E, "Sunny", "A little more light." },
    { 0x0B1D3A, 0xFFFFFF, "Midnight", "Settle into the evening." },
    { 0xF2F2F7, 0x1C1C1E, "Mist", "Soft grey." },
    { 0xFF375F, 0xFFFFFF, "Rose", "A bright finish." },
    { 0xE5F6FF, 0x0A3D62, "Sky", "Room to breathe." },
    { 0x000000, 0xFFFFFF, "Ink", "Everything in its place." },
    { 0xFFFFFF, 0x111111, "Paper again", "Turn on Dark Mode to see another side." },
};
#define BAND_N ((int)(sizeof(k_bands) / sizeof(k_bands[0])))

static lv_obj_t * s_bands[BAND_N];
static lv_obj_t * s_page;
static bool s_dark;
static int32_t s_prog;                  /* 0 = light, 1000 = dark; eased by an animation so colours cross-fade */
static lv_obj_t * s_btn, * s_half, * s_ring, * s_cap;

static lv_color_t mix(uint32_t light, uint32_t dark)
{
    return lv_color_mix(lv_color_hex(dark), lv_color_hex(light), (uint8_t)(s_prog * 255 / 1000));
}

static void paint(void)
{
    lv_obj_set_style_bg_color(s_page, mix(0xFFFFFF, 0x000000), 0);
    for(int i = 0; i < BAND_N; i++) {
        bool flip = k_bands[i].bg == 0xFFFFFF || k_bands[i].bg == 0xF2F2F7 || k_bands[i].bg == 0xE5F6FF;
        lv_obj_set_style_bg_color(s_bands[i], flip ? mix(k_bands[i].bg, 0x1C1C1E) : lv_color_hex(k_bands[i].bg), 0);
        lv_obj_t * t = lv_obj_get_child(s_bands[i], 0);
        lv_obj_t * b = lv_obj_get_child(s_bands[i], 1);
        lv_color_t fg = flip ? mix(k_bands[i].fg, 0xFFFFFF) : lv_color_hex(k_bands[i].fg);
        lv_obj_set_style_text_color(t, fg, 0);
        lv_obj_set_style_text_color(b, fg, 0);
    }
    /* iOS Control Center style appearance button: a ringed disc, half filled, that turns over when toggled */
    lv_color_t ink = mix(0x1C1C1E, 0xFFFFFF);
    lv_obj_set_style_bg_color(s_btn, mix(0xE5E5EA, 0x2C2C2E), 0);
    lv_obj_set_style_border_color(s_ring, ink, 0);
    lv_obj_set_style_arc_color(s_half, ink, LV_PART_INDICATOR);
    lv_obj_set_style_transform_rotation(s_cap, s_prog * 18 / 10, 0);      /* 0..1000 -> 0..180 degrees (0.1 deg units) */
    lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_parent(s_btn), 0), mix(0x636366, 0xAEAEB2), 0);
}

static void prog_exec(void * var, int32_t v)
{
    LV_UNUSED(var);
    s_prog = v;
    paint();
}

static void dark_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s_dark = !s_dark;
    lv_anim_delete(&s_prog, prog_exec);
    lv_anim_t an;
    lv_anim_init(&an);
    lv_anim_set_var(&an, &s_prog);
    lv_anim_set_exec_cb(&an, prog_exec);
    lv_anim_set_values(&an, s_prog, s_dark ? 1000 : 0);
    lv_anim_set_duration(&an, 420);
    lv_anim_set_path_cb(&an, lv_anim_path_ease_in_out);
    lv_anim_start(&an);
}

APP(app_light, "Paper", LV_SYMBOL_EYE_OPEN, 0xE5A800)
{
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(root, lv_color_white(), 0);
    s_dark = false;
    s_prog = 0;

    s_page = lv_obj_create(root);
    lv_obj_remove_style_all(s_page);
    lv_obj_set_size(s_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(s_page, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(s_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(s_page, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_bottom(s_page, OS_GESTURE_H + 10, 0);

    for(int i = 0; i < BAND_N; i++) {
        lv_obj_t * band = ui_box(s_page);
        s_bands[i] = band;
        lv_obj_set_size(band, LV_PCT(100), i == 0 ? 270 : 230);
        lv_obj_remove_flag(band, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_hor(band, 22, 0);
        lv_obj_set_style_pad_top(band, i == 0 ? 78 : 46, 0);
        lv_obj_set_style_pad_row(band, 8, 0);
        lv_obj_set_flex_flow(band, LV_FLEX_FLOW_COLUMN);
        lv_obj_t * t = ui_label(band, k_bands[i].title, &lv_font_montserrat_32, k_bands[i].fg);
        LV_UNUSED(t);
        lv_obj_t * b = ui_label(band, k_bands[i].body, &lv_font_montserrat_16, k_bands[i].fg);
        lv_obj_set_width(b, LV_PCT(100));
        lv_label_set_long_mode(b, LV_LABEL_LONG_MODE_WRAP);
    }

    lv_obj_t * sw_row = ui_box(s_bands[0]);
    lv_obj_set_size(sw_row, LV_PCT(100), 48);
    lv_obj_set_style_pad_top(sw_row, 4, 0);
    lv_obj_t * l = ui_label(sw_row, "Appearance", &lv_font_montserrat_14, 0x636366);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
    s_btn = ui_box(sw_row);
    lv_obj_set_size(s_btn, 44, 44);
    lv_obj_set_style_radius(s_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_btn, LV_OPA_COVER, 0);
    lv_obj_align(s_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_clickable(s_btn, true);
    lv_obj_set_style_transform_scale(s_btn, 236, LV_STATE_PRESSED);
    lv_obj_add_event_cb(s_btn, dark_cb, LV_EVENT_CLICKED, NULL);

    s_cap = ui_box(s_btn);                                  /* the part that turns over */
    lv_obj_set_size(s_cap, 26, 26);
    lv_obj_center(s_cap);
    lv_obj_set_style_transform_pivot_x(s_cap, 13, 0);
    lv_obj_set_style_transform_pivot_y(s_cap, 13, 0);
    lv_obj_remove_flag(s_cap, LV_OBJ_FLAG_CLICKABLE);
    s_ring = ui_box(s_cap);
    lv_obj_set_size(s_ring, 26, 26);
    lv_obj_set_style_radius(s_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_ring, 2, 0);
    lv_obj_set_style_bg_opa(s_ring, LV_OPA_TRANSP, 0);
    s_half = lv_arc_create(s_cap);
    lv_obj_set_size(s_half, 22, 22);
    lv_obj_center(s_half);
    lv_arc_set_rotation(s_half, 0);
    lv_arc_set_bg_angles(s_half, 0, 0);
    lv_arc_set_angles(s_half, 90, 270);                     /* left half of the disc */
    lv_obj_set_style_arc_width(s_half, 11, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_half, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(s_half, false, LV_PART_INDICATOR);
    lv_obj_remove_style(s_half, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(s_half, LV_OBJ_FLAG_CLICKABLE);
    s_prog = 0;
    paint();
}
