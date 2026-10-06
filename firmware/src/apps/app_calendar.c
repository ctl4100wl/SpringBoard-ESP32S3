/**
 * @file app_calendar.c
 * Month view opened on today; tap a day to see its weekday and distance from today.
 */
#include <time.h>
#include "../os/os.h"
#include "../os/ui.h"

static const char * const k_wday[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char * const k_month[] = { "January", "February", "March", "April", "May", "June", "July",
                                        "August", "September", "October", "November", "December" };

static lv_obj_t * s_info;
static lv_obj_t * s_sub;
static struct tm s_today;

static void show_date(int y, int m, int d)
{
    struct tm t = { 0 };
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_hour = 12;
    time_t sel = mktime(&t);

    struct tm today = s_today;
    today.tm_hour = 12;
    today.tm_min = today.tm_sec = 0;
    long days = (long)((sel - mktime(&today)) / 86400);

    lv_label_set_text_fmt(s_info, "%s, %s %d", k_wday[t.tm_wday], k_month[m - 1], d);
    if(days == 0) lv_label_set_text(s_sub, "Today");
    else if(days == 1) lv_label_set_text(s_sub, "Tomorrow");
    else if(days == -1) lv_label_set_text(s_sub, "Yesterday");
    else if(days > 0) lv_label_set_text_fmt(s_sub, "In %ld days", days);
    else lv_label_set_text_fmt(s_sub, "%ld days ago", -days);
}

static void cal_cb(lv_event_t * e)
{
    lv_calendar_date_t d;
    if(lv_calendar_get_pressed_date(lv_event_get_target_obj(e), &d) == LV_RESULT_OK) {
        show_date(d.year, d.month, d.day);
    }
}

static void create(lv_obj_t * root)
{
    time_t now = time(NULL);
    struct tm * lt = localtime(&now);
    if(lt) s_today = *lt;

    lv_obj_t * body = ui_app_scaffold(root, "Calendar");

    lv_obj_t * cal = lv_calendar_create(body);
    lv_obj_set_size(cal, LV_PCT(100), 290);
    lv_calendar_set_today_date(cal, s_today.tm_year + 1900, s_today.tm_mon + 1, s_today.tm_mday);
    lv_calendar_set_month_shown(cal, s_today.tm_year + 1900, s_today.tm_mon + 1);
    lv_obj_t * header = lv_calendar_add_header_arrow(cal);
#ifdef S3_NATIVE_LAYOUT
    /* Theme-created children use physical units; explicitly size the seven rows. */
    lv_obj_set_size(cal, LV_PCT(100), 315);
    lv_obj_set_style_pad_all(cal, 3, 0);
    lv_obj_set_style_pad_row(cal, 3, 0);
    lv_obj_set_height(header, 36);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_t * days = lv_calendar_get_btnmatrix(cal);
    lv_obj_set_style_text_font(days, &lv_font_montserrat_14, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(days, 0, 0);
    lv_obj_set_style_pad_row(days, 0, 0);
    lv_obj_set_style_pad_column(days, 0, 0);
    lv_obj_set_style_border_width(days, 0, LV_PART_ITEMS);
#else
    LV_UNUSED(header);
#endif
    lv_obj_add_event_cb(cal, cal_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * card = ui_card(body);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_pad_row(card, 4, 0);
    s_info = ui_label(card, "", &lv_font_montserrat_18, 0xFFFFFF);
    s_sub = ui_label(card, "", &lv_font_montserrat_14, UI_RED);
    show_date(s_today.tm_year + 1900, s_today.tm_mon + 1, s_today.tm_mday);
#ifdef S3_NATIVE_LAYOUT
    lv_obj_set_width(s_info, LV_PCT(100));
    lv_obj_set_width(s_sub, LV_PCT(100));
    lv_obj_update_layout(body);
    lv_obj_set_height(cal, LV_MAX(222, lv_obj_get_content_height(body) - lv_obj_get_height(card) - 14));
#endif
}

const app_desc_t app_calendar = { "Calendar", LV_SYMBOL_LIST, 0xFF3B30, create };
