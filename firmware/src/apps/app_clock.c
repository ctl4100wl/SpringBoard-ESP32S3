/**
 * @file app_clock.c
 * World clock, stopwatch with laps, countdown timer. Stopwatch/timer keep running while closed.
 */
#include <time.h>
#include "../os/sdk.h"

#define MAX_LAPS 20

typedef struct {
    const char * city;
    int utc_offset;
} zone_t;

static const zone_t k_zones[] = {
    { "Cupertino", -7 }, { "New York", -4 }, { "London", 1 },
    { "Paris", 2 },      { "Dubai", 4 },     { "Tokyo", 9 },
};
#define ZONE_N ((int)(sizeof(k_zones) / sizeof(k_zones[0])))

static struct {
    bool sw_running;
    uint32_t sw_start, sw_accum;
    uint32_t laps[MAX_LAPS];
    int lap_n;

    bool tm_active, tm_running;
    uint32_t tm_total, tm_end, tm_left;
    uint16_t tm_min, tm_sec;
    bool tm_done;
} s = { .tm_min = 1 };

static struct {
    lv_timer_t * tick;
    lv_obj_t * zone_time[ZONE_N];
    lv_obj_t * sw_label, * sw_go, * sw_lap, * lap_list;
    lv_obj_t * tm_idle, * tm_run, * tm_min, * tm_sec, * tm_arc, * tm_label, * tm_pause, * tm_note;
} u;

/*---------------- Stopwatch ----------------*/

static uint32_t sw_elapsed(void)
{
    return s.sw_accum + (s.sw_running ? lv_tick_elaps(s.sw_start) : 0);
}

static void fmt_sw(char * buf, size_t n, uint32_t ms)
{
    lv_snprintf(buf, n, "%02u:%02u.%02u", (unsigned)(ms / 60000), (unsigned)(ms / 1000 % 60), (unsigned)(ms / 10 % 100));
}

static void set_btn_text(lv_obj_t * b, const char * t)
{
    lv_label_set_text(lv_obj_get_child(b, 0), t);
}

static void laps_rebuild(void)
{
    lv_obj_clean(u.lap_list);
    char buf[24];
    for(int i = s.lap_n - 1; i >= 0; i--) {
        lv_obj_t * row = ui_box(u.lap_list);
        lv_obj_set_size(row, LV_PCT(100), 34);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(UI_SEP), 0);
        lv_obj_t * l = lv_label_create(row);
        lv_label_set_text_fmt(l, "Lap %d", i + 1);
        lv_obj_set_style_text_color(l, lv_color_hex(UI_TEXT), 0);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
        uint32_t prev = i > 0 ? s.laps[i - 1] : 0;
        fmt_sw(buf, sizeof(buf), s.laps[i] - prev);
        lv_obj_align(ui_label(row, buf, &lv_font_montserrat_14, UI_TEXT), LV_ALIGN_RIGHT_MID, 0, 0);
    }
}

static void sw_refresh_buttons(void)
{
    set_btn_text(u.sw_go, s.sw_running ? "Stop" : "Start");
    lv_obj_set_style_bg_color(u.sw_go, lv_color_hex(s.sw_running ? 0x4A1210 : 0x0F3D1A), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(u.sw_go, 0), lv_color_hex(s.sw_running ? UI_RED : UI_GREEN), 0);
    set_btn_text(u.sw_lap, s.sw_running || sw_elapsed() == 0 ? "Lap" : "Reset");
}

static void sw_go_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(s.sw_running) {
        s.sw_accum = sw_elapsed();
        s.sw_running = false;
    }
    else {
        s.sw_start = lv_tick_get();
        s.sw_running = true;
    }
    sw_refresh_buttons();
}

static void sw_lap_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(s.sw_running) {
        if(s.lap_n == MAX_LAPS) {
            for(int i = 1; i < MAX_LAPS; i++) s.laps[i - 1] = s.laps[i];
            s.lap_n--;
        }
        s.laps[s.lap_n++] = sw_elapsed();
    }
    else {
        s.sw_accum = 0;
        s.lap_n = 0;
    }
    laps_rebuild();
    sw_refresh_buttons();
}

/*---------------- Timer ----------------*/

static uint32_t tm_remaining(void)
{
    if(!s.tm_running) return s.tm_left;
    int32_t left = (int32_t)(s.tm_end - lv_tick_get());
    return left > 0 ? (uint32_t)left : 0;
}

static void tm_show(void)
{
    lv_obj_set_hidden(u.tm_idle, s.tm_active);
    lv_obj_set_hidden(u.tm_run, !s.tm_active);
    set_btn_text(u.tm_pause, s.tm_running ? "Pause" : "Resume");
    lv_label_set_text(u.tm_note, s.tm_done ? "Time's up!" : "");
}

static void tm_start_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s.tm_min = (uint16_t)lv_roller_get_selected(u.tm_min);
    s.tm_sec = (uint16_t)lv_roller_get_selected(u.tm_sec);
    s.tm_total = (s.tm_min * 60u + s.tm_sec) * 1000u;
    if(s.tm_total == 0) return;
    s.tm_end = lv_tick_get() + s.tm_total;
    s.tm_active = s.tm_running = true;
    s.tm_done = false;
    tm_show();
}

static void tm_pause_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(s.tm_running) {
        s.tm_left = tm_remaining();
        s.tm_running = false;
    }
    else {
        s.tm_end = lv_tick_get() + s.tm_left;
        s.tm_running = true;
    }
    tm_show();
}

static void tm_cancel_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s.tm_active = s.tm_running = false;
    tm_show();
}

/*---------------- Ticker ----------------*/

static void tick_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    char buf[24];

    fmt_sw(buf, sizeof(buf), sw_elapsed());
    lv_label_set_text(u.sw_label, buf);

    if(s.tm_active) {
        uint32_t left = tm_remaining();
        lv_label_set_text_fmt(u.tm_label, "%02u:%02u", (unsigned)((left + 999) / 60000),
                              (unsigned)((left + 999) / 1000 % 60));
        lv_arc_set_value(u.tm_arc, s.tm_total ? (int32_t)(left * 1000ull / s.tm_total) : 0);
        if(left == 0 && s.tm_running) {
            s.tm_active = s.tm_running = false;
            s.tm_done = true;
            tm_show();
        }
    }

    time_t now = time(NULL);
    for(int i = 0; i < ZONE_N; i++) {
        time_t z = now + k_zones[i].utc_offset * 3600;
        struct tm * tm = gmtime(&z);
        if(tm) lv_label_set_text_fmt(u.zone_time[i], "%02d:%02d", tm->tm_hour, tm->tm_min);
    }
}

static void root_delete_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_timer_delete(u.tick);
    u.tick = NULL;
}

/*---------------- Build ----------------*/

static lv_obj_t * round_btn(lv_obj_t * parent, const char * text, uint32_t bg, lv_event_cb_t cb)
{
    lv_obj_t * b = ui_button(parent, text, bg, cb, NULL);
    lv_obj_set_size(b, 78, 78);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    return b;
}

static lv_obj_t * tab_page(lv_obj_t * tv, const char * name, const char * title)
{
    lv_obj_t * p = lv_tabview_add_tab(tv, name);
    lv_obj_set_style_pad_top(p, 40, 0);
    lv_obj_set_style_pad_hor(p, 16, 0);
#ifdef S3_COMPACT
    lv_obj_set_style_pad_top(p, 18, 0);
    lv_obj_set_style_pad_hor(p, 4, 0);
#endif
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(p, 10, 0);
    ui_label(p, title, &lv_font_montserrat_32, 0xFFFFFF);
    return p;
}

static void build_world(lv_obj_t * p)
{
    for(int i = 0; i < ZONE_N; i++) {
        lv_obj_t * row = ui_box(p);
        lv_obj_set_size(row, LV_PCT(100), 52);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(UI_SEP), 0);
        lv_obj_t * c = ui_label(row, k_zones[i].city, &lv_font_montserrat_18, UI_TEXT);
        lv_obj_align(c, LV_ALIGN_LEFT_MID, 0, -8);
        lv_obj_t * o = lv_label_create(row);
        lv_label_set_text_fmt(o, "UTC%+d", k_zones[i].utc_offset);
        lv_obj_set_style_text_color(o, lv_color_hex(UI_DIM), 0);
        lv_obj_align(o, LV_ALIGN_LEFT_MID, 0, 12);
        u.zone_time[i] = ui_label(row, "", &lv_font_montserrat_32, 0xFFFFFF);
        lv_obj_align(u.zone_time[i], LV_ALIGN_RIGHT_MID, 0, 0);
    }
}

static void build_stopwatch(lv_obj_t * p)
{
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    u.sw_label = ui_label(p, "00:00.00", &lv_font_montserrat_48, 0xFFFFFF);

    lv_obj_t * btns = ui_box(p);
    lv_obj_set_size(btns, LV_PCT(100), 80);
    u.sw_lap = round_btn(btns, "Lap", 0x333333, sw_lap_cb);
    lv_obj_align(u.sw_lap, LV_ALIGN_LEFT_MID, 0, 0);
    u.sw_go = round_btn(btns, "Start", 0x0F3D1A, sw_go_cb);
    lv_obj_align(u.sw_go, LV_ALIGN_RIGHT_MID, 0, 0);

    u.lap_list = ui_box(p);
    lv_obj_set_width(u.lap_list, LV_PCT(100));
    lv_obj_set_flex_grow(u.lap_list, 1);
    lv_obj_set_flex_flow(u.lap_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollable(u.lap_list, true);
    lv_obj_set_clickable(u.lap_list, true);
    lv_obj_set_scroll_dir(u.lap_list, LV_DIR_VER);
    laps_rebuild();
    sw_refresh_buttons();
}

static lv_obj_t * num_roller(lv_obj_t * parent, int sel, const char * unit)
{
    static char opts[60 * 3 + 1];
    if(!opts[0]) {
        char * w = opts;
        for(int i = 0; i < 60; i++) w += lv_snprintf(w, 4, i == 59 ? "%02d" : "%02d\n", i);
    }
    lv_obj_t * col = ui_box(parent);
    lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t * r = lv_roller_create(col);
    lv_roller_set_options(r, opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(r, 3);
    lv_roller_set_selected(r, (uint32_t)sel, LV_ANIM_OFF);
    lv_obj_set_width(r, 90);
    lv_obj_set_style_text_font(r, &lv_font_montserrat_24, 0);
    ui_label(col, unit, &lv_font_montserrat_14, UI_DIM);
    return r;
}

static void build_timer(lv_obj_t * p)
{
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    u.tm_idle = ui_box(p);
    lv_obj_set_size(u.tm_idle, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(u.tm_idle, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(u.tm_idle, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(u.tm_idle, 20, 0);
    lv_obj_t * rollers = ui_box(u.tm_idle);
    lv_obj_set_size(rollers, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(rollers, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(rollers, 20, 0);
    u.tm_min = num_roller(rollers, s.tm_min, "min");
    u.tm_sec = num_roller(rollers, s.tm_sec, "sec");
    round_btn(u.tm_idle, "Start", 0x0F3D1A, tm_start_cb);

    u.tm_run = ui_box(p);
    lv_obj_set_size(u.tm_run, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(u.tm_run, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(u.tm_run, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(u.tm_run, 16, 0);
    u.tm_arc = lv_arc_create(u.tm_run);
    lv_obj_set_size(u.tm_arc, 190, 190);
    lv_arc_set_rotation(u.tm_arc, 270);
    lv_arc_set_bg_angles(u.tm_arc, 0, 360);
    lv_arc_set_range(u.tm_arc, 0, 1000);
    lv_obj_remove_style(u.tm_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_color(u.tm_arc, lv_color_hex(UI_ORANGE), LV_PART_INDICATOR);
    lv_obj_set_clickable(u.tm_arc, false);
    u.tm_label = ui_label(u.tm_arc, "", &lv_font_montserrat_40, 0xFFFFFF);
    lv_obj_center(u.tm_label);
    lv_obj_t * btns = ui_box(u.tm_run);
    lv_obj_set_size(btns, LV_PCT(100), 80);
    lv_obj_align(round_btn(btns, "Cancel", 0x333333, tm_cancel_cb), LV_ALIGN_LEFT_MID, 0, 0);
    u.tm_pause = round_btn(btns, "Pause", 0x4A2A05, tm_pause_cb);
    lv_obj_align(u.tm_pause, LV_ALIGN_RIGHT_MID, 0, 0);

    u.tm_note = ui_label(p, "", &lv_font_montserrat_18, UI_ORANGE);
    tm_show();
}

static void create(lv_obj_t * root)
{
    lv_obj_t * tv = lv_tabview_create(root);
    lv_obj_set_size(tv, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(tv, lv_color_black(), 0);
    lv_tabview_set_tab_bar_position(tv, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(tv, 48 + OS_GESTURE_H);
#ifdef S3_COMPACT
    lv_tabview_set_tab_bar_size(tv, 22);
#endif
    lv_obj_t * bar = lv_tabview_get_tab_bar(tv);
    lv_obj_set_style_pad_bottom(bar, OS_GESTURE_H, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x111111), 0);

    build_world(tab_page(tv, "World", "World Clock"));
    build_stopwatch(tab_page(tv, "Stopwatch", "Stopwatch"));
    build_timer(tab_page(tv, "Timer", "Timer"));
    lv_tabview_set_active(tv, 1, LV_ANIM_OFF);

    u.tick = lv_timer_create(tick_cb, 50, NULL);
    tick_cb(NULL);
    lv_obj_add_event_cb(root, root_delete_cb, LV_EVENT_DELETE, NULL);
}

const app_desc_t app_clock = { "Clock", LV_SYMBOL_LOOP, 0x1C1C1E, create };

/*---------------- Widget ----------------*/

static void widget_tick(lv_timer_t * t)
{
    lv_obj_t * box = lv_timer_get_user_data(t);
    static const char * const wd[] = { "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY" };
    time_t now = time(NULL);
    struct tm * tm = localtime(&now);
    if(!tm) return;
    int h = tm->tm_hour % 12;
    lv_label_set_text(lv_obj_get_child(box, 0), wd[tm->tm_wday]);
    lv_label_set_text_fmt(lv_obj_get_child(box, 1), "%d", tm->tm_mday);
    lv_label_set_text_fmt(lv_obj_get_child(box, 2), "%d:%02d", h == 0 ? 12 : h, tm->tm_min);
    lv_label_set_text(lv_obj_get_child(box, 3), s.sw_running ? LV_SYMBOL_PLAY " Stopwatch running" :
                      (s.tm_running ? LV_SYMBOL_BELL " Timer running" : ""));
}

static void widget_create(lv_obj_t * box)
{
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    ui_label(box, "", &lv_font_montserrat_12, UI_RED);
    ui_label(box, "", &lv_font_montserrat_36, 0xFFFFFF);
    ui_label(box, "", &lv_font_montserrat_20, 0xFFFFFF);
    ui_label(box, "", &lv_font_montserrat_12, UI_DIM);
    widget_tick(sdk_timer(box, widget_tick, 1000, box));
}

static void lock_tick(lv_timer_t * t)
{
    lv_obj_t * slot = lv_timer_get_user_data(t);
    lv_label_set_text(lv_obj_get_child(slot, 0), s.sw_running ? LV_SYMBOL_PLAY " Stopwatch" :
                      (s.tm_running ? LV_SYMBOL_BELL " Timer" : LV_SYMBOL_LOOP " Clock"));
    lv_label_set_text(lv_obj_get_child(slot, 1), s.sw_running || s.tm_running ? "Running" : "No alarms");
}

static void widget_lock(lv_obj_t * slot)
{
    lv_obj_set_flex_flow(slot, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(slot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    ui_label(slot, "", &lv_font_montserrat_14, 0xFFFFFF);
    ui_label(slot, "", &lv_font_montserrat_12, 0xE5E5EA);
    lock_tick(sdk_timer(slot, lock_tick, 1000, slot));
}

const widget_desc_t widget_clock = { "Clock", &app_clock, WIDGET_SMALL, widget_create, widget_lock, false };
