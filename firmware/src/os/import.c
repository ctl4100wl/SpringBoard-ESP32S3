/**
 * @file import.c
 * Drop an image file on the simulator window: a glass sheet asks where to use it
 * (Home, Lock, both) and sets it as the wallpaper.
 *
 * The window system delivers the path through os_drop_poll() (hal.c on the PC). On the device the
 * same sheet can be opened with import_offer(path) from an SD card browser.
 */
#include <string.h>
#include "sdk.h"

#define SHEET_W     272
#define PREVIEW_W   232
#define PREVIEW_H   120

static struct {
    lv_obj_t * layer;       /* dim + sheet, NULL when closed */
    lv_obj_t * sheet;
    lv_draw_buf_t * thumb;
    char path[300];
} m;

static bool has_ext(const char * path)
{
    const char * dot = strrchr(path, '.');
    if(!dot) return false;
    static const char * const ok[] = { ".png", ".jpg", ".jpeg", ".bmp" };
    for(unsigned i = 0; i < sizeof(ok) / sizeof(ok[0]); i++) {
        if(lv_strcmp(dot, ok[i]) == 0) return true;
        /* case-insensitive compare without locale tables */
        const char * a = dot, * b = ok[i];
        while(*a && *b && ((*a | 0x20) == *b)) { a++; b++; }
        if(!*a && !*b) return true;
    }
    return false;
}

static void close_gone_cb(lv_obj_t * o)
{
    LV_UNUSED(o);
    if(m.layer) lv_obj_delete(m.layer);
    m.layer = m.sheet = NULL;
    if(m.thumb) {
        lv_draw_buf_destroy(m.thumb);
        m.thumb = NULL;
    }
}

static void sheet_close(void)
{
    if(!m.layer) return;
    lv_obj_set_clickable(m.layer, false);
    fx_to(m.sheet, FX_Y, (float)lv_obj_get_height(m.layer), 0.30f, 1.0f, 0, NULL);
    fx_to(m.layer, FX_OPA, 0, 0.22f, 1.0f, 60, close_gone_cb);
}

static void choose_cb(lv_event_t * e)
{
    int where = (int)(intptr_t)lv_event_get_user_data(e);    /* 0 home, 1 lock, 2 both */
    if(where == 0 || where == 2) {
        sdk_set_str("wall.home", m.path);
        os_settings.wallpaper = OS_WALL_CUSTOM;
    }
    if(where == 1 || where == 2) {
        sdk_set_str("wall.lock", m.path);
        os_settings.lock_wall = OS_WALL_CUSTOM;
    }
    sheet_close();
    shell_settings_apply();
    os_save();
    sdk_toast("Wallpaper Set", where == 0 ? "Home Screen updated." : where == 1 ? "Lock Screen updated." :
              "Home Screen and Lock Screen updated.");
}

static void cancel_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    sheet_close();
}

static lv_obj_t * pick_button(lv_obj_t * parent, const char * text, uint32_t color, int where)
{
    lv_obj_t * b = ui_button(parent, text, color, where < 0 ? cancel_cb : choose_cb, (void *)(intptr_t)where);
    lv_obj_set_height(b, 40);
    lv_obj_set_style_radius(b, 20, 0);
    return b;
}

void import_offer(const char * path)
{
    if(!path || !path[0]) return;
    if(!has_ext(path)) {
        sdk_toast("Import", "Drop a PNG, JPG or BMP image.");
        return;
    }
    if(m.layer) close_gone_cb(NULL);

    int32_t W = lv_display_get_horizontal_resolution(lv_display_get_default());
    int32_t H = lv_display_get_vertical_resolution(lv_display_get_default());
    lv_strncpy(m.path, path, sizeof(m.path) - 1);
    m.thumb = sdk_image_load(path, PREVIEW_W, PREVIEW_H);
    if(!m.thumb) {
        sdk_toast("Import", "Could not decode that image (PNG, JPG or BMP).");
        return;
    }

    m.layer = ui_box(lv_layer_top());           /* above the lock screen, banners and Control Center */
    lv_obj_set_size(m.layer, W, H);
    lv_obj_set_clickable(m.layer, true);
    lv_obj_set_style_bg_color(m.layer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(m.layer, LV_OPA_40, 0);
    lv_obj_add_event_cb(m.layer, cancel_cb, LV_EVENT_CLICKED, NULL);

    m.sheet = ui_box(m.layer);
    lv_obj_set_size(m.sheet, SHEET_W, LV_SIZE_CONTENT);
    lv_obj_align(m.sheet, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_set_clickable(m.sheet, true);        /* taps on the sheet itself do not dismiss it */
    lv_obj_set_style_radius(m.sheet, 28, 0);
    ui_glass(m.sheet, 0x2C2C2E, LV_OPA_60, 30);
    lv_obj_set_style_pad_all(m.sheet, 14, 0);
    lv_obj_set_style_pad_row(m.sheet, 8, 0);
    lv_obj_set_flex_flow(m.sheet, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(m.sheet, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title = ui_label(m.sheet, "Use as Wallpaper", &lv_font_montserrat_16, 0xFFFFFF);
    LV_UNUSED(title);

    lv_obj_t * frame = ui_box(m.sheet);
    lv_obj_set_size(frame, PREVIEW_W, PREVIEW_H);
    lv_obj_set_style_radius(frame, 16, 0);
    lv_obj_set_style_clip_corner(frame, true, 0);
    lv_obj_t * img = lv_image_create(frame);
    lv_image_set_src(img, m.thumb);

    lv_obj_t * row = ui_box(m.sheet);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t * b;
    b = pick_button(row, "Home", UI_BLUE, 0);
    lv_obj_set_flex_grow(b, 1);
    b = pick_button(row, "Lock", UI_BLUE, 1);
    lv_obj_set_flex_grow(b, 1);
    b = pick_button(row, "Both", UI_GREEN, 2);
    lv_obj_set_flex_grow(b, 1);
    b = pick_button(m.sheet, "Cancel", 0x48484A, -1);
    lv_obj_set_width(b, LV_PCT(100));

    lv_obj_update_layout(m.layer);
    fx_set(m.layer, FX_OPA, 0);
    fx_to(m.layer, FX_OPA, 255, 0.22f, 1.0f, 0, NULL);
    fx_set(m.sheet, FX_Y, (float)H);
    fx_to(m.sheet, FX_Y, 0, 0.50f, 0.74f, 0, NULL);
}

static void poll_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    char path[300];
    if(os_drop_poll(path, sizeof(path))) import_offer(path);
}

void import_init(void)
{
    lv_timer_create(poll_cb, 150, NULL);
}
