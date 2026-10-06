/**
 * @file app_sketch.c
 * Finger painting on a canvas. The drawing buffer is kept, so the sketch survives relaunch.
 */
#include "../os/os.h"
#include "../os/ui.h"

#ifdef S3_COMPACT
#define CANVAS_W    120
#define CANVAS_H    88
#else
#define CANVAS_W    288
#define CANVAS_H    340
#endif
#define PAPER       0xFFFFFF

static const uint32_t k_colors[] = { 0x000000, 0xFF3B30, 0xFF9500, 0x34C759, 0x0A84FF, 0xAF52DE };
#define COLOR_N ((int)(sizeof(k_colors) / sizeof(k_colors[0])))

static lv_draw_buf_t * s_buf;
static int s_color;             /* COLOR_N = eraser */
static lv_obj_t * s_canvas;
static lv_obj_t * s_chip[COLOR_N + 1];
static lv_point_t s_last;
static bool s_down;

static void chips_refresh(void)
{
    for(int i = 0; i <= COLOR_N; i++) lv_obj_set_style_border_width(s_chip[i], i == s_color ? 3 : 0, 0);
}

static void chip_cb(lv_event_t * e)
{
    s_color = (int)(intptr_t)lv_event_get_user_data(e);
    chips_refresh();
}

static void clear_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_canvas_fill_bg(s_canvas, lv_color_hex(PAPER), LV_OPA_COVER);
}

static void stroke(lv_point_t a, lv_point_t b)
{
    lv_layer_t layer;
    lv_canvas_init_layer(s_canvas, &layer);
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    bool eraser = s_color == COLOR_N;
    d.color = lv_color_hex(eraser ? PAPER : k_colors[s_color]);
    d.width = eraser ? 18 : 5;
    d.round_start = 1;
    d.round_end = 1;
    d.p1.x = a.x;
    d.p1.y = a.y;
    d.p2.x = b.x;
    d.p2.y = b.y;
    lv_draw_line(&layer, &d);
    lv_canvas_finish_layer(s_canvas, &layer);
}

static void canvas_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING &&
       code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) return;
    if(!lv_indev_active()) return;
    lv_point_t p;
    lv_area_t a;
    lv_indev_get_point(lv_indev_active(), &p);
    lv_obj_get_coords(s_canvas, &a);
    p.x -= a.x1;
    p.y -= a.y1;

    if(code == LV_EVENT_PRESSED) {
        s_last = p;
        s_down = true;
        stroke(p, p);
    }
    else if(code == LV_EVENT_PRESSING && s_down) {
        if(p.x != s_last.x || p.y != s_last.y) stroke(s_last, p);
        s_last = p;
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_down = false;
    }
}

static void create(lv_obj_t * root)
{
    lv_obj_t * body = ui_app_scaffold(root, "Sketch");
    lv_obj_set_scrollable(body, false);

    bool fresh = s_buf == NULL;
    if(fresh) {
#ifdef S3_NATIVE_LAYOUT
        s_buf = lv_draw_buf_create(s3_native_px(CANVAS_W), s3_native_px(CANVAS_H), LV_COLOR_FORMAT_RGB565, 0);
#else
        s_buf = lv_draw_buf_create(CANVAS_W, CANVAS_H, LV_COLOR_FORMAT_RGB565, 0);
#endif
    }
    if(!s_buf) {
        ui_label(body, "Not enough memory for the canvas.", &lv_font_montserrat_14, UI_RED);
        return;
    }

    s_canvas = lv_canvas_create(body);
    lv_canvas_set_draw_buf(s_canvas, s_buf);
    if(fresh) lv_canvas_fill_bg(s_canvas, lv_color_hex(PAPER), LV_OPA_COVER);
    lv_obj_set_style_radius(s_canvas, 12, 0);
    lv_obj_set_style_clip_corner(s_canvas, true, 0);
    lv_obj_set_clickable(s_canvas, true);
    lv_obj_set_press_lock(s_canvas, true);
    lv_obj_add_event_cb(s_canvas, canvas_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * bar = ui_box(body);
    lv_obj_set_size(bar, LV_PCT(100), 34);
#ifdef S3_COMPACT
    lv_obj_set_height(bar, 16);
#endif
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    for(int i = 0; i <= COLOR_N; i++) {
        lv_obj_t * c = ui_box(bar);
        lv_obj_set_size(c, 28, 28);
#ifdef S3_COMPACT
        lv_obj_set_size(c, 11, 11);
#endif
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(c, lv_color_hex(i < COLOR_N ? k_colors[i] : 0xE5E5EA), 0);
        lv_obj_set_style_border_color(c, lv_color_hex(i == 0 ? 0x8E8E93 : 0xFFFFFF), 0);
        lv_obj_set_clickable(c, true);
        lv_obj_add_event_cb(c, chip_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        if(i == COLOR_N) lv_obj_center(ui_label(c, LV_SYMBOL_BACKSPACE, &lv_font_montserrat_12, 0x000000));
        s_chip[i] = c;
    }
    lv_obj_t * clr = ui_button(bar, LV_SYMBOL_TRASH, UI_GROUP, clear_cb, NULL);
    lv_obj_set_size(clr, 40, 30);
#ifdef S3_COMPACT
    lv_obj_set_size(clr, 16, 16);
#endif
    chips_refresh();
}

const app_desc_t app_sketch = { "Sketch", LV_SYMBOL_EDIT, 0xAF52DE, create };
