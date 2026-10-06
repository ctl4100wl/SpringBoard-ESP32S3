/**
 * @file widgets_extra.c
 * More home screen widgets: battery ring, calendar, now playing, notes, steps. Each one is a few lines
 * of SDK code (see README). They open their linked app when tapped.
 */
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "../os/sdk.h"

extern const app_desc_t app_settings, app_calendar, app_music, app_notes, app_clock;

/*---------------- Battery (small) ----------------*/

static void battery_tick(lv_timer_t * t)
{
    lv_obj_t * box = lv_timer_get_user_data(t);
    lv_obj_t * arc = lv_obj_get_child(box, 0);
    int pct = 100 - (int)((lv_tick_get() / 4000) % 70);    /* demo drain: no battery on the PC */
    lv_arc_set_value(arc, pct);
    lv_obj_set_style_arc_color(arc, lv_color_hex(pct < 30 ? UI_ORANGE : UI_GREEN), LV_PART_INDICATOR);
    lv_label_set_text_fmt(lv_obj_get_child(arc, 0), "%d%%", pct);
}

WIDGET(widget_battery, "Battery", &app_settings, WIDGET_SMALL, NULL)
{
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_t * arc = lv_arc_create(box);
    lv_obj_set_size(arc, 76, 76);
    lv_obj_center(arc);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 100);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc, 8, 0);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x3A3A3C), 0);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    lv_obj_set_clickable(arc, false);
    lv_obj_center(ui_label(arc, "", &lv_font_montserrat_16, 0xFFFFFF));
    battery_tick(sdk_timer(box, battery_tick, 2000, box));
}

/*---------------- Calendar (medium) ----------------*/

static const char * const k_wd[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

static void calendar_fill(lv_obj_t * box)
{
    time_t now = time(NULL);
    struct tm * tm = localtime(&now);
    lv_obj_clean(box);
    lv_obj_t * wd = ui_label(box, tm ? k_wd[tm->tm_wday] : "MON", &lv_font_montserrat_14, UI_RED);
    lv_obj_align(wd, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * day = ui_label(box, "", &lv_font_montserrat_48, 0xFFFFFF);
    lv_label_set_text_fmt(day, "%d", tm ? tm->tm_mday : 9);
    lv_obj_align(day, LV_ALIGN_TOP_LEFT, 0, 16);

    static const char * const ev[3] = { "Design review", "Lunch with Sam", "Gym" };
    static const char * const at[3] = { "10:00", "12:30", "18:00" };
    lv_obj_t * list = ui_box(box);
    lv_obj_set_size(list, 150, LV_SIZE_CONTENT);
    lv_obj_align(list, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 6, 0);
    static const uint32_t col[3] = { 0x0A84FF, 0x30D158, 0xFF9F0A };
    for(int i = 0; i < 3; i++) {
        lv_obj_t * r = ui_box(list);
        lv_obj_set_size(r, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(r, 6, 0);
        lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t * bar = ui_box(r);
        lv_obj_set_size(bar, 3, 26);
        lv_obj_set_style_radius(bar, 2, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(col[i]), 0);
        lv_obj_t * c = ui_box(r);
        lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
        ui_label(c, ev[i], &lv_font_montserrat_12, 0xFFFFFF);
        ui_label(c, at[i], &lv_font_montserrat_12, UI_DIM);
    }
}

static void calendar_tick(lv_timer_t * t)
{
    calendar_fill(lv_timer_get_user_data(t));
}

WIDGET(widget_calendar, "Calendar", &app_calendar, WIDGET_MEDIUM, NULL)
{
    calendar_fill(box);
    sdk_timer(box, calendar_tick, 60000, box);
}

/*---------------- Now playing (medium) ----------------*/

WIDGET(widget_music, "Now Playing", &app_music, WIDGET_MEDIUM, NULL)
{
    lv_obj_t * art = ui_box(box);
    lv_obj_set_size(art, 66, 66);
    lv_obj_align(art, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(art, 14, 0);
    ui_icon_style(art, 0xFF375F);
    lv_obj_center(ui_label(art, LV_SYMBOL_AUDIO, &lv_font_montserrat_28, 0xFFFFFF));

    lv_obj_t * t = ui_label(box, "Under Mentality", &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_align(t, LV_ALIGN_TOP_LEFT, 80, 4);
    lv_obj_t * a = ui_label(box, "TUYU", &lv_font_montserrat_12, UI_DIM);
    lv_obj_align(a, LV_ALIGN_TOP_LEFT, 80, 26);

    lv_obj_t * bar = lv_bar_create(box);
    lv_obj_set_size(bar, 150, 4);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 80, 50);
    lv_bar_set_value(bar, 38, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x48484A), 0);
    lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_INDICATOR);

    lv_obj_t * ctl = ui_label(box, LV_SYMBOL_PREV "     " LV_SYMBOL_PLAY "     " LV_SYMBOL_NEXT, &lv_font_montserrat_16,
                              0xFFFFFF);
    lv_obj_align(ctl, LV_ALIGN_BOTTOM_LEFT, 84, 0);
}

/*---------------- Notes (small) ----------------*/

WIDGET(widget_notes, "Notes", &app_notes, WIDGET_SMALL, NULL)
{
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(box, 4, 0);
    lv_obj_t * h = ui_label(box, LV_SYMBOL_EDIT "  Notes", &lv_font_montserrat_14, 0xFFD60A);
    LV_UNUSED(h);
    const char * saved = sdk_get_str("notes.last", "");
    lv_obj_t * body = ui_label(box, saved[0] ? saved : "Tap to write your first note", &lv_font_montserrat_12,
                               0xFFFFFF);
    lv_obj_set_width(body, LV_PCT(100));
    lv_label_set_long_mode(body, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_height(body, 62);
}

/*---------------- Steps (medium): bar sparkline ----------------*/

WIDGET(widget_steps, "Activity", &app_clock, WIDGET_MEDIUM, NULL)
{
    lv_obj_t * h = ui_label(box, LV_SYMBOL_CHARGE "  Steps", &lv_font_montserrat_14, UI_GREEN);
    lv_obj_align(h, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * n = ui_label(box, "7,842", &lv_font_montserrat_28, 0xFFFFFF);
    lv_obj_align(n, LV_ALIGN_TOP_LEFT, 0, 20);
    static const int v[7] = { 40, 62, 35, 80, 55, 90, 70 };
    for(int i = 0; i < 7; i++) {
        lv_obj_t * b = ui_box(box);
        lv_obj_set_size(b, 12, v[i] * 54 / 100);
        lv_obj_align(b, LV_ALIGN_BOTTOM_RIGHT, -i * 20, 0);
        lv_obj_set_style_radius(b, 4, 0);
        lv_obj_set_style_bg_opa(b, i == 0 ? LV_OPA_COVER : LV_OPA_50, 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(UI_GREEN), 0);
    }
    lv_obj_t * g = ui_label(box, "Goal 10,000", &lv_font_montserrat_12, UI_DIM);
    lv_obj_align(g, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

/*---------------- Curved, resizable widgets ----------------
 * Both lay themselves out for Small / Medium / Large (sdk_widget_size()); the home screen springs the frame to
 * the new size and calls the create function again, so everything below is just "draw for this size". */

extern const app_desc_t app_weather;

static void arc_anim_exec(void * a, int32_t v)
{
    lv_arc_set_value((lv_obj_t *)a, v);
}

/* One activity ring: background track + coloured progress that sweeps in */
static lv_obj_t * ring(lv_obj_t * parent, int32_t d, int32_t width, uint32_t color, int value, uint32_t delay)
{
    lv_obj_t * a = lv_arc_create(parent);
    lv_obj_set_size(a, d, d);
    lv_arc_set_rotation(a, 270);
    lv_arc_set_bg_angles(a, 0, 360);
    lv_arc_set_range(a, 0, 100);
    lv_arc_set_value(a, 0);
    lv_obj_remove_style(a, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(a, width, 0);
    lv_obj_set_style_arc_width(a, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(a, lv_color_hex(color), 0);
    lv_obj_set_style_arc_opa(a, LV_OPA_20, 0);
    lv_obj_set_style_arc_color(a, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(a, true, 0);
    lv_obj_set_style_arc_rounded(a, true, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(a, 0, 0);
    lv_obj_set_clickable(a, false);
    lv_anim_t an;
    lv_anim_init(&an);
    lv_anim_set_var(&an, a);
    lv_anim_set_exec_cb(&an, arc_anim_exec);
    lv_anim_set_values(&an, 0, value);
    lv_anim_set_duration(&an, 800);
    lv_anim_set_delay(&an, delay);
    lv_anim_set_path_cb(&an, lv_anim_path_ease_out);
    lv_anim_start(&an);
    return a;
}

static void rings_at(lv_obj_t * parent, int32_t d, int32_t w, int32_t cx, int32_t cy)
{
    static const uint32_t col[3] = { 0xFF2D55, 0x9BE31E, 0x32D6E8 };
    static const int val[3] = { 72, 46, 83 };
    for(int i = 0; i < 3; i++) {
        int32_t dd = d - i * (w * 2 + 3);
        lv_obj_t * r = ring(parent, dd, w, col[i], val[i], (uint32_t)i * 120);
        lv_obj_set_pos(r, cx - dd / 2, cy - dd / 2);
    }
}

static lv_obj_t * stat(lv_obj_t * parent, const char * name, const char * value, uint32_t color)
{
    lv_obj_t * c = ui_box(parent);
    lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_t * n = ui_label(c, name, &lv_font_montserrat_12, color);
    LV_UNUSED(n);
    lv_obj_t * v = ui_label(c, value, &lv_font_montserrat_16, 0xFFFFFF);
    LV_UNUSED(v);
    return c;
}

WIDGET_RESIZABLE(widget_rings, "Activity Rings", &app_clock, WIDGET_SMALL, NULL)
{
    widget_size_t sz = sdk_widget_size();
    lv_obj_update_layout(box);
    int32_t cw = lv_obj_get_content_width(box), ch = lv_obj_get_content_height(box);
    lv_obj_set_style_pad_all(box, 12, 0);
    if(sz == WIDGET_SMALL) {
        int32_t d = LV_MIN(cw, ch);
        rings_at(box, d, 10, cw / 2, ch / 2);
    }
    else if(sz == WIDGET_MEDIUM) {
        int32_t d = LV_MIN(ch, 118);
        rings_at(box, d, 11, d / 2, ch / 2);
        lv_obj_t * col = ui_box(box);
        lv_obj_set_size(col, cw - d - 16, ch);
        lv_obj_set_pos(col, d + 16, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        stat(col, "Move", "432/600 CAL", 0xFF2D55);
        stat(col, "Exercise", "23/50 MIN", 0x9BE31E);
        stat(col, "Stand", "10/12 HRS", 0x32D6E8);
    }
    else {
        int32_t d = LV_MIN(cw, 190);
        rings_at(box, d, 14, cw / 2, d / 2 + 4);
        lv_obj_t * row = ui_box(box);
        lv_obj_set_size(row, cw, LV_SIZE_CONTENT);
        lv_obj_set_pos(row, 0, d + 18);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        stat(row, "Move", "432 CAL", 0xFF2D55);
        stat(row, "Exercise", "23 MIN", 0x9BE31E);
        stat(row, "Stand", "10 HRS", 0x32D6E8);
        static const int wk[7] = { 60, 85, 40, 95, 70, 55, 72 };
        int32_t top = d + 78;
        int32_t bh = LV_MAX(20, ch - top);
        for(int i = 0; i < 7; i++) {
            lv_obj_t * b = ui_box(box);
            lv_obj_set_size(b, (cw - 6 * 6) / 7, wk[i] * bh / 100);
            lv_obj_set_pos(b, i * ((cw - 6 * 6) / 7 + 6), top + bh - wk[i] * bh / 100);
            lv_obj_set_style_radius(b, 4, 0);
            lv_obj_set_style_bg_opa(b, i == 6 ? LV_OPA_COVER : LV_OPA_40, 0);
            lv_obj_set_style_bg_color(b, lv_color_hex(0xFF2D55), 0);
        }
    }
}

/* Daylight: the sun's path as a curved arc with the sun riding it */
static void sun_dot(lv_obj_t * parent, int32_t cx, int32_t cy, int32_t r, float t, int32_t size)
{
    float a = 3.14159265f * (1.0f - t);
    lv_obj_t * glow = ui_box(parent);
    lv_obj_set_size(glow, size * 2, size * 2);
    lv_obj_set_style_radius(glow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(glow, LV_OPA_30, 0);
    lv_obj_set_style_bg_color(glow, lv_color_hex(0xFFD60A), 0);
    int32_t x = cx + (int32_t)(r * cosf(a)), y = cy - (int32_t)(r * sinf(a));
    lv_obj_set_pos(glow, x - size, y - size);
    lv_obj_t * s = ui_box(parent);
    lv_obj_set_size(s, size, size);
    lv_obj_set_style_radius(s, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s, lv_color_hex(0xFFD60A), 0);
    lv_obj_set_pos(s, x - size / 2, y - size / 2);
    fx_set(s, FX_OPA, 0);
    fx_to(s, FX_OPA, 255, 0.4f, 1.0f, 300, NULL);
}

static void sun_arc(lv_obj_t * parent, int32_t r, int32_t cx, int32_t cy, int32_t width)
{
    lv_obj_t * a = lv_arc_create(parent);
    lv_obj_set_size(a, r * 2 + width, r * 2 + width);
    lv_obj_set_pos(a, cx - r - width / 2, cy - r - width / 2);
    lv_arc_set_bg_angles(a, 180, 360);
    lv_arc_set_range(a, 0, 100);
    lv_arc_set_value(a, 0);
    lv_obj_remove_style(a, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(a, width, 0);
    lv_obj_set_style_arc_color(a, lv_color_hex(0xFFD60A), 0);
    lv_obj_set_style_arc_opa(a, LV_OPA_40, 0);
    lv_obj_set_style_arc_rounded(a, true, 0);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(a, 0, 0);
    lv_obj_set_clickable(a, false);
}

WIDGET_RESIZABLE(widget_daylight, "Daylight", &app_weather, WIDGET_MEDIUM, NULL)
{
    widget_size_t sz = sdk_widget_size();
    lv_obj_update_layout(box);
    int32_t cw = lv_obj_get_content_width(box), ch = lv_obj_get_content_height(box);
    time_t now = time(NULL);
    struct tm * tm = localtime(&now);
    float hr = tm ? tm->tm_hour + tm->tm_min / 60.0f : 12.0f;
    float t = LV_CLAMP(0.02f, (hr - 6.2f) / (18.7f - 6.2f), 0.98f);
    uint32_t col = 0xFFFFFF;

    if(sz == WIDGET_SMALL) {
        int32_t r = cw / 2 - 8;
        sun_arc(box, r, cw / 2, r + 10, 5);
        sun_dot(box, cw / 2, r + 10, r, t, 12);
        lv_obj_t * temp = ui_label(box, "24\xC2\xB0", &lv_font_montserrat_28, col);
        lv_obj_align(temp, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_t * sub = ui_label(box, "Sunset 6:41", &lv_font_montserrat_12, 0xFFD60A);
        lv_obj_align(sub, LV_ALIGN_BOTTOM_RIGHT, 0, -2);
    }
    else if(sz == WIDGET_MEDIUM) {
        int32_t r = LV_MIN(cw / 2 - 12, ch - 38);
        sun_arc(box, r, cw / 2, ch - 22, 6);
        sun_dot(box, cw / 2, ch - 22, r, t, 14);
        lv_obj_t * rise = ui_label(box, "Sunrise\n6:12 AM", &lv_font_montserrat_12, UI_DIM);
        lv_obj_align(rise, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_t * set = ui_label(box, "Sunset\n6:41 PM", &lv_font_montserrat_12, UI_DIM);
        lv_obj_set_style_text_align(set, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(set, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        lv_obj_t * temp = ui_label(box, "24\xC2\xB0", &lv_font_montserrat_36, col);
        lv_obj_align(temp, LV_ALIGN_BOTTOM_MID, 0, -8);
    }
    else {
        int32_t r = cw / 2 - 12;
        sun_arc(box, r, cw / 2, r + 36, 7);
        sun_dot(box, cw / 2, r + 36, r, t, 16);
        lv_obj_t * temp = ui_label(box, "24\xC2\xB0", &lv_font_montserrat_48, col);
        lv_obj_align(temp, LV_ALIGN_TOP_MID, 0, r + 8);
        lv_obj_t * line = ui_label(box, "Sunny  H:27\xC2\xB0 L:15\xC2\xB0", &lv_font_montserrat_14, UI_DIM);
        lv_obj_align(line, LV_ALIGN_TOP_MID, 0, r + 62);
        lv_obj_t * row = ui_box(box);
        lv_obj_set_size(row, cw, LV_SIZE_CONTENT);
        lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        static const char * const hrs[5] = { "Now", "3 PM", "6 PM", "9 PM", "12 AM" };
        static const char * const tp[5] = { "24\xC2\xB0", "26\xC2\xB0", "21\xC2\xB0", "18\xC2\xB0", "16\xC2\xB0" };
        for(int i = 0; i < 5; i++) {
            lv_obj_t * c = ui_box(row);
            lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(c, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            ui_label(c, hrs[i], &lv_font_montserrat_12, UI_DIM);
            ui_label(c, tp[i], &lv_font_montserrat_16, 0xFFFFFF);
        }
    }
}
