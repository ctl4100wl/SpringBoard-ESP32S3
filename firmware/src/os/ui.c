/**
 * @file ui.c
 */
#include "ui.h"
#include "os.h"
#include <stdio.h>
#include <stdlib.h>

const lv_font_t * ui_font_vietnamese(const lv_font_t * base)
{
#ifdef S3_COMPACT
    /* Native small-screen layout; large desktop fonts would clip on 128px panels. */
    if(base->line_height > 24) return &lv_font_montserrat_18;
    if(base->line_height > 16) return &lv_font_montserrat_12;
#endif
#if !defined(ESP_PLATFORM) && LV_USE_TINY_TTF
    static uint8_t * font_data;
    static size_t font_size;
    static bool attempted;
    static struct { const lv_font_t * base; lv_font_t * fallback; lv_font_t combined; } fonts[16];
    if(!attempted) {
        attempted = true;
        FILE * f = fopen("lvgl/scripts/generators/built_in_font/DejaVuSans.ttf", "rb");
        if(!f) f = fopen("../lvgl/scripts/generators/built_in_font/DejaVuSans.ttf", "rb");
        if(f) {
            fseek(f, 0, SEEK_END);
            long n = ftell(f);
            rewind(f);
            if(n > 0 && n < 2000000) {
                font_data = malloc((size_t)n);
                if(font_data && fread(font_data, 1, (size_t)n, f) == (size_t)n) font_size = (size_t)n;
            }
            fclose(f);
        }
    }
    if(!font_size) return base;
    for(int i = 0; i < 16; i++) {
        if(fonts[i].base == base) return &fonts[i].combined;
        if(fonts[i].base) continue;
        fonts[i].fallback = lv_tiny_ttf_create_data(font_data, font_size, base->line_height);
        if(!fonts[i].fallback) return base;
        fonts[i].base = base;
        fonts[i].combined = *base;
        fonts[i].combined.fallback = fonts[i].fallback;
        return &fonts[i].combined;
    }
#endif
    return base;
}

void ui_icon_style(lv_obj_t * obj, uint32_t color)
{
    lv_color_t c = lv_color_hex(color);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, lv_color_lighten(c, 40), 0);
    lv_obj_set_style_bg_grad_color(obj, c, 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
}

lv_obj_t * ui_box(lv_obj_t * parent)
{
    lv_obj_t * o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    return o;
}

lv_obj_t * ui_label(lv_obj_t * parent, const char * text, const lv_font_t * font, uint32_t color)
{
    lv_obj_t * l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, ui_font_vietnamese(font), 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

/* Large title + a vertically scrolling body below it. Returns the body. */
lv_obj_t * ui_app_scaffold(lv_obj_t * root, const char * title)
{
#ifdef S3_COMPACT
    lv_obj_set_style_pad_top(root, 18, 0);
    lv_obj_set_style_pad_hor(root, 4, 0);
    lv_obj_set_style_pad_row(root, 4, 0);
#else
    lv_obj_set_style_pad_top(root, 40, 0);
    lv_obj_set_style_pad_hor(root, 16, 0);
    lv_obj_set_style_pad_row(root, 10, 0);
#endif
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    ui_label(root, title, &lv_font_montserrat_32, 0xFFFFFF);

    lv_obj_t * body = lv_obj_create(root);
    lv_obj_remove_style_all(body);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 14, 0);
    lv_obj_set_style_pad_bottom(body, OS_GESTURE_H + 8, 0);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    return body;
}

lv_obj_t * ui_card(lv_obj_t * parent)
{
    lv_obj_t * c = ui_box(parent);
    lv_obj_set_size(c, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(c, 12, 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(c, lv_color_hex(UI_GROUP), 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    return c;
}

lv_obj_t * ui_row(lv_obj_t * card, const char * sym, uint32_t accent, const char * text)
{
    lv_obj_t * row = ui_box(card);
    lv_obj_set_size(row, LV_PCT(100), 46);
    lv_obj_set_style_pad_hor(row, 12, 0);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if(sym) {
        lv_obj_t * badge = ui_box(row);
        lv_obj_set_size(badge, 28, 28);
        lv_obj_set_style_radius(badge, 7, 0);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(badge, lv_color_hex(accent), 0);
        lv_obj_center(ui_label(badge, sym, &lv_font_montserrat_14, 0xFFFFFF));
    }
    lv_obj_t * lbl = ui_label(row, text, &lv_font_montserrat_16, UI_TEXT);
    lv_obj_set_flex_grow(lbl, 1);
    return row;
}

lv_obj_t * ui_button(lv_obj_t * parent, const char * text, uint32_t bg, lv_event_cb_t cb, void * user_data)
{
    lv_obj_t * b = lv_button_create(parent);
    lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
    lv_obj_set_style_radius(b, 12, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    if(cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user_data);
    lv_obj_t * l = ui_label(b, text, &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_center(l);
    return b;
}

static void kb_event_cb(lv_event_t * e)
{
    lv_obj_t * kb = lv_event_get_target_obj(e);
    ui_keyboard_hide(kb);
}

static void kb_slide_exec(void * obj, int32_t y)
{
    lv_obj_set_style_translate_y((lv_obj_t *)obj, y, 0);
}

static void kb_hidden_cb(lv_anim_t * a)
{
    lv_obj_t * kb = lv_anim_get_user_data(a);
    lv_obj_set_hidden(kb, true);
    lv_obj_set_style_translate_y(kb, 0, 0);
}

static void kb_slide(lv_obj_t * kb, int32_t from, int32_t to, bool hide)
{
    fx_stop(kb);
    lv_anim_delete(kb, kb_slide_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, kb);
    lv_anim_set_user_data(&a, kb);
    lv_anim_set_exec_cb(&a, kb_slide_exec);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, hide ? 190 : 260);
    lv_anim_set_path_cb(&a, hide ? lv_anim_path_ease_in : lv_anim_path_ease_out);
    if(hide) lv_anim_set_completed_cb(&a, kb_hidden_cb);
    lv_anim_start(&a);
}

void ui_keyboard_show(lv_obj_t * kb)
{
    if(!kb) return;
    lv_anim_delete(kb, kb_slide_exec);
    int32_t from = lv_obj_get_style_translate_y(kb, 0);
    if(lv_obj_is_hidden(kb)) {
        lv_obj_set_hidden(kb, false);
        from = lv_obj_get_height(kb) + OS_GESTURE_H;
        lv_obj_set_style_translate_y(kb, from, 0);
    }
    if(from) {
        fx_set(kb, FX_Y, (float)from);
        fx_to(kb, FX_Y, 0, 0.38f, 1.0f, 0, NULL);
    }
}

void ui_keyboard_hide(lv_obj_t * kb)
{
    if(!kb || lv_obj_is_hidden(kb)) return;
    kb_slide(kb, lv_obj_get_style_translate_y(kb, 0), lv_obj_get_height(kb) + OS_GESTURE_H, true);
}

bool ui_keyboard_dismiss_visible(lv_obj_t * root)
{
    if(!root || lv_obj_is_hidden(root)) return false;
    uint32_t count = lv_obj_get_child_count(root);
    for(uint32_t i = count; i > 0; i--)
        if(ui_keyboard_dismiss_visible(lv_obj_get_child(root, (int32_t)i - 1))) return true;
    if(lv_obj_check_type(root, &lv_keyboard_class)) {
        ui_keyboard_hide(root);
        return true;
    }
    return false;
}

/* Floating keyboard sitting just above the system gesture strip; hidden until shown */
lv_obj_t * ui_keyboard(lv_obj_t * root, lv_obj_t * textarea)
{
    lv_obj_t * kb = lv_keyboard_create(root);
    lv_obj_set_floating(kb, true);
    lv_obj_set_size(kb, lv_display_get_horizontal_resolution(lv_display_get_default()), 190);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -OS_GESTURE_H);
    lv_obj_set_style_text_font(textarea, ui_font_vietnamese(&lv_font_montserrat_16), 0);
    lv_obj_set_style_text_font(kb, ui_font_vietnamese(&lv_font_montserrat_16), 0);
    lv_keyboard_set_textarea(kb, textarea);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_CANCEL, NULL);
    lv_obj_set_hidden(kb, true);
    return kb;
}

static int32_t s_glass_bezel = 22;
static int32_t s_glass_strength = 85;
static bool s_glass_chroma = true;

void ui_lens_settings_get(int32_t * bezel, int32_t * strength, bool * chroma)
{
    if(bezel) *bezel = s_glass_bezel;
    if(strength) *strength = s_glass_strength;
    if(chroma) *chroma = s_glass_chroma;
}

void ui_lens_settings_set(int32_t bezel, int32_t strength, bool chroma)
{
    s_glass_bezel = LV_CLAMP(6, bezel, 40);
    s_glass_strength = LV_CLAMP(0, strength, 100);
    s_glass_chroma = chroma;
}

#ifdef ESP_PLATFORM
static bool runtime_blur, runtime_lens;
#else
static bool runtime_blur = OS_FX_BLUR, runtime_lens = OS_FX_LENS;
#endif
bool ui_blur_enabled(void) { return runtime_blur; }
bool ui_lens_enabled(void) { return runtime_lens; }
#ifdef ESP_PLATFORM
extern bool (*lv_draw_blur_enabled_hook)(void);
#endif
void ui_effects_set(bool blur, bool lens)
{
 runtime_blur = blur || lens; runtime_lens = lens;
#ifdef ESP_PLATFORM
 lv_draw_blur_enabled_hook = ui_blur_enabled;
#endif
 lv_obj_invalidate(lv_screen_active());
}

void ui_glass(lv_obj_t * obj, uint32_t tint, lv_opa_t opa, int32_t blur)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(tint), 0);
    if(!ui_blur_enabled()) {
        lv_obj_set_style_blur_backdrop(obj, false, 0);
        lv_obj_set_style_bg_opa(obj, (lv_opa_t)LV_MIN(255, opa + 110), 0); return;
    }
#if OS_FX_BLUR
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_blur_backdrop(obj, true, 0);
    lv_obj_set_style_blur_radius(obj, blur, 0);
#else
    LV_UNUSED(blur);
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)LV_MIN(255, opa + 110), 0);
#endif
}

void ui_glass_set(lv_obj_t * obj, float p, int32_t max_blur, lv_opa_t opa)
{
    p = LV_CLAMP(0.0f, p, 1.0f);
    if(!ui_blur_enabled()) {
        lv_obj_set_style_blur_backdrop(obj, false, 0);
        lv_obj_set_style_bg_opa(obj, (lv_opa_t)LV_MIN(255, (opa + 110)*p), 0); return;
    }
#if OS_FX_BLUR
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)(opa * p), 0);
    int32_t r = ((int32_t)(max_blur * p) / 3) * 3;      /* quantize: fewer distinct radii, fewer re-blurs */
    lv_obj_set_style_blur_radius(obj, r, 0);
    lv_obj_set_style_blur_backdrop(obj, r > 0, 0);
#else
    LV_UNUSED(max_blur);
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)LV_MIN(255, (opa + 110) * p), 0);   /* no blur: denser tint instead */
#endif
}

/* One box-blur pass over an A8 plane (running sum, edge clamped), horizontal or vertical */
static void box_pass(uint8_t * buf, int32_t w, int32_t h, int32_t stride, int32_t r, bool vertical)
{
    int32_t n = vertical ? h : w, lines = vertical ? w : h;
    int32_t step = vertical ? stride : 1, lstep = vertical ? 1 : stride;
    uint8_t * tmp = lv_malloc((size_t)n);
    if(!tmp) return;
    for(int32_t l = 0; l < lines; l++) {
        uint8_t * p = buf + l * lstep;
        int32_t sum = 0;
        for(int32_t i = -r; i <= r; i++) sum += p[LV_CLAMP(0, i, n - 1) * step];
        int32_t div = 2 * r + 1;
        for(int32_t i = 0; i < n; i++) {
            tmp[i] = (uint8_t)(sum / div);
            sum += p[LV_MIN(i + r + 1, n - 1) * step] - p[LV_MAX(i - r, 0) * step];
        }
        for(int32_t i = 0; i < n; i++) p[i * step] = tmp[i];
    }
    lv_free(tmp);
}

lv_draw_buf_t * ui_text_halo(const char * text, const lv_font_t * font, int32_t radius, int32_t pad)
{
    lv_obj_t * box = ui_box(lv_layer_sys());
    lv_obj_t * l = ui_label(box, text, font, 0xFFFFFF);
    lv_obj_set_style_pad_all(box, pad, 0);
    lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(box, -2000, 0);
    lv_obj_update_layout(box);
    lv_draw_buf_t * buf = lv_snapshot_take(box, LV_COLOR_FORMAT_A8);
    lv_obj_delete(box);
    LV_UNUSED(l);
    if(!buf) return NULL;
    int32_t w = (int32_t)buf->header.w, h = (int32_t)buf->header.h, stride = (int32_t)buf->header.stride;
    int32_t r = LV_MAX(1, radius / 3);
#ifdef S3_NATIVE_LAYOUT
    r = LV_MAX(1, s3_native_px(radius) / 3);
#endif
    for(int i = 0; i < 3; i++) {
        box_pass(buf->data, w, h, stride, r, false);
        box_pass(buf->data, w, h, stride, r, true);
    }
    return buf;
}

/*---------------- Navigation stack ----------------*/

static void nav_gone_cb(lv_obj_t * page)
{
    lv_obj_delete_async(page);
}

#define NAV_TAG ((void *)0x4E41)   /* user data that marks a page made by ui_nav_push */

/* Topmost pushed page of an app window, NULL if the app is on its first screen */
lv_obj_t * ui_nav_top(lv_obj_t * root)
{
    if(!root) return NULL;
    for(int32_t i = (int32_t)lv_obj_get_child_count(root) - 1; i >= 0; i--) {
        lv_obj_t * c = lv_obj_get_child(root, i);
        if(lv_obj_get_user_data(c) == NAV_TAG && !lv_obj_has_state(c, LV_STATE_DISABLED)) return c;
    }
    return NULL;
}

static void nav_back_cb(lv_event_t * e)
{
    lv_obj_t * page = lv_event_get_user_data(e);
    lv_obj_add_state(page, LV_STATE_DISABLED);          /* leaving: no longer the top page */
    lv_obj_t * under = lv_obj_get_sibling(page, -1);
    if(under && lv_obj_is_floating(under)) fx_to(under, FX_X, 0, FX_SNAPPY, 0, NULL);
    fx_to(page, FX_X, (float)lv_obj_get_width(page), FX_SNAPPY, 0, nav_gone_cb);
}

lv_obj_t * ui_nav_push(lv_obj_t * root, const char * title, const char * back)
{
    int32_t w = lv_display_get_horizontal_resolution(lv_display_get_default());
    /* Previous top page slides a bit to the left, like UINavigationController */
    lv_obj_t * prev = lv_obj_get_child(root, -1);

    lv_obj_t * page = ui_box(root);
    lv_obj_set_user_data(page, NAV_TAG);
    lv_obj_set_floating(page, true);
    /* Cover the whole root, including its padding */
    lv_obj_set_size(page, w, lv_display_get_vertical_resolution(lv_display_get_default()));
    lv_obj_set_pos(page, -lv_obj_get_style_pad_left(root, 0), -lv_obj_get_style_pad_top(root, 0));
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(page, lv_color_black(), 0);
    lv_obj_set_clickable(page, true);
    lv_obj_set_style_pad_top(page, 34, 0);
    lv_obj_set_style_pad_hor(page, 16, 0);
    lv_obj_set_style_pad_row(page, 6, 0);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * head = ui_box(page);
    lv_obj_set_size(head, LV_PCT(100), 30);
    lv_obj_t * b = ui_box(head);
    lv_obj_set_size(b, LV_SIZE_CONTENT, 30);
    lv_obj_set_clickable(b, true);
    lv_obj_set_ext_click_area(b, 10);
    lv_obj_add_event_cb(b, nav_back_cb, LV_EVENT_CLICKED, page);
    lv_obj_set_style_opa(b, LV_OPA_50, LV_STATE_PRESSED);
    lv_obj_t * bl = ui_label(b, "", &lv_font_montserrat_16, UI_BLUE);
    lv_label_set_text_fmt(bl, LV_SYMBOL_LEFT " %s", back ? back : "Back");
    lv_obj_align(bl, LV_ALIGN_LEFT_MID, 0, 0);

    ui_label(page, title, &lv_font_montserrat_28, 0xFFFFFF);

    lv_obj_t * body = lv_obj_create(page);
    lv_obj_remove_style_all(body);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 8, 0);
    lv_obj_set_style_pad_top(body, 6, 0);
    lv_obj_set_style_pad_bottom(body, OS_GESTURE_H + 12, 0);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);

    fx_set(page, FX_X, (float)w);
    fx_to(page, FX_X, 0, FX_SNAPPY, 0, NULL);
    if(prev && lv_obj_is_floating(prev)) fx_to(prev, FX_X, -w / 3.0f, FX_SNAPPY, 0, NULL);
    return body;
}

lv_obj_t * ui_section(lv_obj_t * body, const char * caption)
{
    if(caption) {
        lv_obj_t * l = ui_label(body, caption, &lv_font_montserrat_12, UI_DIM);
        lv_obj_set_style_pad_left(l, 12, 0);
        lv_obj_set_style_pad_top(l, 8, 0);
    }
    return ui_card(body);
}

lv_obj_t * ui_nav_row(lv_obj_t * card, const char * sym, uint32_t accent, const char * text, const char * value,
                      lv_event_cb_t cb, void * user_data)
{
    lv_obj_t * row = ui_row(card, sym, accent, text);
    if(value) ui_label(row, value, &lv_font_montserrat_14, UI_DIM);
    ui_label(row, LV_SYMBOL_RIGHT, &lv_font_montserrat_12, 0x5A5A5E);
    lv_obj_set_clickable(row, true);
    lv_obj_set_style_radius(row, 12, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(UI_SEP), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_STATE_PRESSED);
    if(cb) lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, user_data);
    return row;
}

lv_obj_t * ui_switch_row(lv_obj_t * card, const char * sym, uint32_t accent, const char * text, bool on,
                         lv_event_cb_t cb, void * user_data)
{
    lv_obj_t * row = ui_row(card, sym, accent, text);
    lv_obj_t * sw = lv_switch_create(row);
    lv_obj_set_size(sw, 46, 26);
    lv_obj_set_style_bg_color(sw, lv_color_hex(UI_GREEN), LV_PART_INDICATOR | LV_STATE_CHECKED);
    if(on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    if(cb) lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, user_data);
    return sw;
}

/* Pop the top page (same animation as the back button). `from_swipe` = page already follows the finger. */
bool ui_nav_pop(lv_obj_t * root)
{
    lv_obj_t * page = ui_nav_top(root);
    if(!page) return false;
    lv_obj_add_state(page, LV_STATE_DISABLED);
    lv_obj_t * under = lv_obj_get_sibling(page, -1);
    if(under && lv_obj_is_floating(under)) fx_to(under, FX_X, 0, FX_SNAPPY, 0, NULL);
    fx_to(page, FX_X, (float)lv_obj_get_width(page), FX_SNAPPY, 0, nav_gone_cb);
    return true;
}
