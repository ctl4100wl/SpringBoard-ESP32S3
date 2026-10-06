/**
 * @file app_weather.c
 * Live forecast from open-meteo.com (free, no API key) via sdk_http_get.
 * Saved locations (add by searching, remove, switch), and the home screen widgets which can follow the app's
 * selected location, stick to one location, or swap through all of them. Falls back to sample data when offline.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../os/sdk.h"

#define REFRESH_MS  (10 * 60 * 1000)
#define HOURS       12
#define DAYS        5
#define LOC_MAX     12
#define SWAP_MS     7000

typedef struct {
    char name[32];
    char lat[16], lon[16];
    int sample_temp, sample_code;
} loc_t;

static const loc_t k_defaults[] = {
    { "Cupertino", "37.32", "-122.03", 24, 0 },
    { "London",    "51.51", "-0.13",   13, 61 },
    { "Tokyo",     "35.68", "139.69",  21, 2 },
    { "Hanoi",     "21.03", "105.85",  30, 80 },
    { "Reykjavik", "64.15", "-21.94",  -2, 71 },
};

typedef struct {
    bool live;
    uint32_t fetched;
    bool pending;
    int temp, code;
    int hourly[HOURS];
    int hi[DAYS], lo[DAYS];
} wx_t;

static loc_t s_loc[LOC_MAX];
static int s_nloc;
static wx_t s_wx[LOC_MAX];
static int s_city;
static lv_obj_t * s_root, * s_view, * s_scroll, * s_locs_body;

extern const app_desc_t app_weather;
static void build_view(void);
static void build_locs(void);

/*---------------- Saved locations ----------------*/

static void loc_save(void)
{
    char k[16];
    sdk_set_int("wx.n", s_nloc);
    for(int i = 0; i < s_nloc; i++) {
        lv_snprintf(k, sizeof(k), "wx.%dn", i);
        sdk_set_str(k, s_loc[i].name);
        lv_snprintf(k, sizeof(k), "wx.%da", i);
        sdk_set_str(k, s_loc[i].lat);
        lv_snprintf(k, sizeof(k), "wx.%do", i);
        sdk_set_str(k, s_loc[i].lon);
    }
}

static void loc_load(void)
{
    int n = sdk_get_int("wx.n", 0);
    char k[16];
    s_nloc = 0;
    for(int i = 0; i < n && i < LOC_MAX; i++) {
        lv_snprintf(k, sizeof(k), "wx.%dn", i);
        const char * nm = sdk_get_str(k, "");
        if(!nm[0]) continue;
        loc_t * l = &s_loc[s_nloc];
        lv_memzero(l, sizeof(*l));
        lv_strncpy(l->name, nm, sizeof(l->name) - 1);
        lv_snprintf(k, sizeof(k), "wx.%da", i);
        lv_strncpy(l->lat, sdk_get_str(k, "0"), sizeof(l->lat) - 1);
        lv_snprintf(k, sizeof(k), "wx.%do", i);
        lv_strncpy(l->lon, sdk_get_str(k, "0"), sizeof(l->lon) - 1);
        unsigned hsh = 7;
        for(const char * c = l->name; *c; c++) hsh = hsh * 31 + (unsigned char)*c;
        l->sample_temp = 8 + (int)(hsh % 22);
        l->sample_code = (hsh % 3) ? 2 : 0;
        for(int d = 0; d < (int)(sizeof(k_defaults) / sizeof(k_defaults[0])); d++) {
            if(!strcmp(k_defaults[d].name, l->name)) {
                l->sample_temp = k_defaults[d].sample_temp;
                l->sample_code = k_defaults[d].sample_code;
            }
        }
        s_nloc++;
    }
    if(s_nloc == 0) {
        s_nloc = (int)(sizeof(k_defaults) / sizeof(k_defaults[0]));
        memcpy(s_loc, k_defaults, sizeof(k_defaults));
    }
    s_city = LV_CLAMP(0, sdk_get_int("weather.city", 0), s_nloc - 1);
}

static void loc_add(const char * name, double lat, double lon)
{
    if(s_nloc >= LOC_MAX) {
        sdk_toast("Weather", "Location limit reached (12). Remove one first.");
        return;
    }
    loc_t * l = &s_loc[s_nloc];
    lv_memzero(l, sizeof(*l));
    lv_strncpy(l->name, name, sizeof(l->name) - 1);
    lv_snprintf(l->lat, sizeof(l->lat), "%.2f", lat);
    lv_snprintf(l->lon, sizeof(l->lon), "%.2f", lon);
    unsigned hsh = 7;
    for(const char * c = l->name; *c; c++) hsh = hsh * 31 + (unsigned char)*c;
    l->sample_temp = 8 + (int)(hsh % 22);
    l->sample_code = (hsh % 3) ? 2 : 0;
    lv_memzero(&s_wx[s_nloc], sizeof(wx_t));
    s_nloc++;
    loc_save();
}

static void loc_remove(int i)
{
    if(s_nloc <= 1 || i < 0 || i >= s_nloc) return;
    for(int j = i + 1; j < s_nloc; j++) {
        s_loc[j - 1] = s_loc[j];
        s_wx[j - 1] = s_wx[j];
    }
    s_nloc--;
    lv_memzero(&s_wx[s_nloc], sizeof(wx_t));
    if(s_city >= s_nloc) s_city = s_nloc - 1;
    if(s_city > i) s_city--;
    sdk_set_int("weather.city", s_city);
    int wl = sdk_get_int("wx.wloc", 0);
    if(wl >= s_nloc || wl == i) sdk_set_int("wx.wloc", 0);
    loc_save();
}

/* Which location a widget shows: 0 = the app's selection, 1 = one fixed location, 2 = swapping through all */
static int widget_loc(void)
{
    if(!s_nloc) loc_load();
    int mode = sdk_get_int("wx.wmode", 0);
    if(mode == 1) return LV_CLAMP(0, sdk_get_int("wx.wloc", 0), s_nloc - 1);
    if(mode == 2) return (int)((lv_tick_get() / SWAP_MS) % (uint32_t)s_nloc);
    return LV_CLAMP(0, sdk_get_int("weather.city", 0), s_nloc - 1);
}

/*---------------- Data ----------------*/

static void sample_fill(int i)
{
    wx_t * w = &s_wx[i];
    if(w->live) return;
    const loc_t * c = &s_loc[i];
    w->temp = c->sample_temp;
    w->code = c->sample_code;
    for(int h = 0; h < HOURS; h++) w->hourly[h] = c->sample_temp + (h % 5) - 2 - (h > 6 ? 2 : 0);
    for(int d = 0; d < DAYS; d++) {
        w->hi[d] = c->sample_temp + 3 + (d * 7 % 3);
        w->lo[d] = c->sample_temp - 6 + (d * 5 % 3);
    }
}

static void condition(int code, const char ** text, const char ** sym, uint32_t * top, uint32_t * bottom)
{
    if(code == 0)       { *text = "Clear";         *sym = LV_SYMBOL_IMAGE;   *top = 0x1E6FD9; *bottom = 0x6FB1FC; }
    else if(code <= 3)  { *text = "Partly Cloudy"; *sym = LV_SYMBOL_UPLOAD;  *top = 0x2E5C8A; *bottom = 0x8AB4D8; }
    else if(code <= 48) { *text = "Fog";           *sym = LV_SYMBOL_MINUS;   *top = 0x4B5563; *bottom = 0x9CA3AF; }
    else if(code <= 67) { *text = "Rain";          *sym = LV_SYMBOL_TINT;    *top = 0x3C4A5C; *bottom = 0x6D7F95; }
    else if(code <= 77) { *text = "Snow";          *sym = LV_SYMBOL_CHARGE;  *top = 0x52617A; *bottom = 0xA9B6C9; }
    else if(code <= 82) { *text = "Showers";       *sym = LV_SYMBOL_TINT;    *top = 0x34495E; *bottom = 0x5D7A94; }
    else                { *text = "Thunderstorm";  *sym = LV_SYMBOL_WARNING; *top = 0x2C2240; *bottom = 0x5B4A7A; }
}

static void on_reply(bool ok, const char * body, void * ud)
{
    int i = (int)(intptr_t)ud;
    if(i < 0 || i >= s_nloc) return;
    wx_t * w = &s_wx[i];
    w->pending = false;
    w->fetched = lv_tick_get();

    double v, arr[HOURS];
    const char * cur = sdk_json_find(body, "current");
    if(!ok || !cur || !sdk_json_num(cur, "temperature_2m", &v)) return;
    w->live = true;
    w->temp = (int)lround(v);
    if(sdk_json_num(cur, "weather_code", &v)) w->code = (int)v;

    int n = sdk_json_num_array(sdk_json_find(body, "hourly"), "temperature_2m", arr, HOURS);
    for(int h = 0; h < n; h++) w->hourly[h] = (int)lround(arr[h]);
    const char * daily = sdk_json_find(body, "daily");
    n = sdk_json_num_array(daily, "temperature_2m_max", arr, DAYS);
    for(int d = 0; d < n; d++) w->hi[d] = (int)lround(arr[d]);
    n = sdk_json_num_array(daily, "temperature_2m_min", arr, DAYS);
    for(int d = 0; d < n; d++) w->lo[d] = (int)lround(arr[d]);
}

static void app_reply(bool ok, const char * body, void * ud)
{
    on_reply(ok, body, ud);
    sdk_pull_refresh_done(s_scroll);
    if((int)(intptr_t)ud == s_city) build_view();
    if(s_locs_body) build_locs();
}

/* Fetch if never fetched or stale; `owner` scopes the callback to a live UI */
static void fetch(int i, sdk_http_cb_t cb, lv_obj_t * owner)
{
    if(i < 0 || i >= s_nloc) return;
    wx_t * w = &s_wx[i];
    sample_fill(i);
    if(w->pending || (w->fetched && lv_tick_elaps(w->fetched) < REFRESH_MS)) return;
    char url[320];
    lv_snprintf(url, sizeof(url),
                "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s"
                "&current=temperature_2m,weather_code&hourly=temperature_2m&forecast_hours=%d"
                "&daily=temperature_2m_max,temperature_2m_min&forecast_days=%d&timezone=auto",
                s_loc[i].lat, s_loc[i].lon, HOURS, DAYS);
    w->pending = true;
    sdk_http_get(url, cb, (void *)(intptr_t)i, owner);
}

/*---------------- App ----------------*/

static void chip_cb(lv_event_t * e)
{
    s_city = (int)(intptr_t)lv_event_get_user_data(e);
    sdk_set_int("weather.city", s_city);
    fetch(s_city, app_reply, s_view);
    build_view();
}

static lv_obj_t * deg(lv_obj_t * parent, int v, const lv_font_t * f, uint32_t color)
{
    char buf[12];
    lv_snprintf(buf, sizeof(buf), "%d\xC2\xB0", v);
    return ui_label(parent, buf, f, color);
}

static lv_obj_t * glass(lv_obj_t * parent)
{
    lv_obj_t * c = ui_card(parent);
    lv_obj_set_style_bg_color(c, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_20, 0);
    return c;
}

static void locs_page_cb(lv_event_t * e);

static void build_view(void)
{
    const wx_t * w = &s_wx[s_city];
    const char * text, * sym;
    uint32_t top, bottom;
    condition(w->code, &text, &sym, &top, &bottom);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(top), 0);
    lv_obj_set_style_bg_grad_color(s_root, lv_color_hex(bottom), 0);
    lv_obj_set_style_bg_grad_dir(s_root, LV_GRAD_DIR_VER, 0);

    lv_obj_clean(s_view);

    lv_obj_t * chips = ui_box(s_view);
    lv_obj_set_size(chips, LV_PCT(100), 34);
    lv_obj_set_flex_flow(chips, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(chips, 6, 0);
    lv_obj_set_scrollable(chips, true);
    lv_obj_set_clickable(chips, true);
    lv_obj_set_scroll_dir(chips, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(chips, LV_SCROLLBAR_MODE_OFF);
    for(int i = 0; i < s_nloc; i++) {
        lv_obj_t * b = ui_button(chips, s_loc[i].name, 0xFFFFFF, chip_cb, (void *)(intptr_t)i);
        lv_obj_set_height(b, 30);
        lv_obj_set_style_radius(b, 15, 0);
        lv_obj_set_style_bg_opa(b, i == s_city ? LV_OPA_60 : LV_OPA_20, 0);
        lv_obj_set_style_text_font(lv_obj_get_child(b, 0), &lv_font_montserrat_14, 0);
    }
    lv_obj_t * manage = ui_button(chips, LV_SYMBOL_LIST " Locations", 0x000000, locs_page_cb, NULL);
    lv_obj_set_height(manage, 30);
    lv_obj_set_style_radius(manage, 15, 0);
    lv_obj_set_style_bg_opa(manage, LV_OPA_30, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(manage, 0), &lv_font_montserrat_14, 0);

    lv_obj_t * hero = ui_box(s_view);
    lv_obj_set_size(hero, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hero, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    ui_label(hero, s_loc[s_city].name, &lv_font_montserrat_28, 0xFFFFFF);
    deg(hero, w->temp, &lv_font_montserrat_48, 0xFFFFFF);
    ui_label(hero, text, &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_t * hl = lv_label_create(hero);
    lv_label_set_text_fmt(hl, "H:%d\xC2\xB0  L:%d\xC2\xB0", w->hi[0], w->lo[0]);
    lv_obj_set_style_text_color(hl, lv_color_white(), 0);
    ui_label(hero, w->pending ? "Updating..." : (w->live ? LV_SYMBOL_WIFI " Live from open-meteo.com" :
                                                 LV_SYMBOL_WARNING " Sample forecast"),
             &lv_font_montserrat_12, 0xE0E8F0);

    lv_obj_t * hourly = glass(s_view);
    lv_obj_set_flex_flow(hourly, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(hourly, 10, 0);
    lv_obj_set_style_pad_column(hourly, 18, 0);
    lv_obj_set_scrollable(hourly, true);
    lv_obj_set_clickable(hourly, true);
    lv_obj_set_scroll_dir(hourly, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(hourly, LV_SCROLLBAR_MODE_OFF);
    for(int i = 0; i < HOURS; i++) {
        lv_obj_t * col = ui_box(hourly);
        lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(col, 6, 0);
        lv_obj_t * hr = lv_label_create(col);
        if(i == 0) lv_label_set_text(hr, "Now");
        else lv_label_set_text_fmt(hr, "+%dh", i);
        lv_obj_set_style_text_color(hr, lv_color_white(), 0);
        ui_label(col, sym, &lv_font_montserrat_18, 0xFFFFFF);
        deg(col, w->hourly[i], &lv_font_montserrat_14, 0xFFFFFF);
    }

    static const char * const days[DAYS] = { "Today", "+1d", "+2d", "+3d", "+4d" };
    int lo_all = w->lo[0], hi_all = w->hi[0];
    for(int d = 1; d < DAYS; d++) {
        lo_all = LV_MIN(lo_all, w->lo[d]);
        hi_all = LV_MAX(hi_all, w->hi[d]);
    }
    lv_obj_t * list = glass(s_view);
    lv_obj_set_style_pad_hor(list, 12, 0);
    for(int d = 0; d < DAYS; d++) {
        lv_obj_t * row = ui_box(list);
        lv_obj_set_size(row, LV_PCT(100), 38);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 10, 0);
        lv_obj_set_width(ui_label(row, days[d], &lv_font_montserrat_16, 0xFFFFFF), 56);
        deg(row, w->lo[d], &lv_font_montserrat_14, 0xD0D8E0);
        lv_obj_t * bar = lv_bar_create(row);
        lv_obj_set_flex_grow(bar, 1);
        lv_obj_set_height(bar, 5);
        lv_bar_set_range(bar, lo_all - 1, hi_all + 1);
        lv_bar_set_mode(bar, LV_BAR_MODE_RANGE);
        lv_bar_set_start_value(bar, w->lo[d], LV_ANIM_OFF);
        lv_bar_set_value(bar, w->hi[d], LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar, lv_color_hex(UI_ORANGE), LV_PART_INDICATOR);
        deg(row, w->hi[d], &lv_font_montserrat_14, 0xFFFFFF);
    }
}

static void refresh_cb(lv_obj_t * body, void * ud)
{
    LV_UNUSED(body);
    LV_UNUSED(ud);
    s_wx[s_city].fetched = 0;   /* force a new request */
    fetch(s_city, app_reply, s_view);
    if(!s_wx[s_city].pending) sdk_pull_refresh_done(s_scroll);
    else build_view();
}

/*---------------- Locations: switch, remove, add, and what the widgets show ----------------*/

static lv_obj_t * s_search_ta, * s_results;
static lv_obj_t * s_kb;
static struct {
    char name[32];
    double lat, lon;
    char sub[40];
} s_res[6];
static int s_res_n;

static void loc_row_cb(lv_event_t * e)
{
    s_city = (int)(intptr_t)lv_event_get_user_data(e);
    sdk_set_int("weather.city", s_city);
    fetch(s_city, app_reply, s_view);
    build_view();
    build_locs();
}

static void loc_del_cb(lv_event_t * e)
{
    loc_remove((int)(intptr_t)lv_event_get_user_data(e));
    build_view();
    build_locs();
}

static void wmode_cb(lv_event_t * e)
{
    sdk_set_int("wx.wmode", (int)(intptr_t)lv_event_get_user_data(e));
    build_locs();
}

static void wloc_cb(lv_event_t * e)
{
    sdk_set_int("wx.wloc", (int)(intptr_t)lv_event_get_user_data(e));
    build_locs();
}

static void add_page_cb(lv_event_t * e);

static void locs_deleted_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s_locs_body = NULL;
}

static lv_obj_t * pill(lv_obj_t * parent, const char * text, bool on, lv_event_cb_t cb, intptr_t ud)
{
    lv_obj_t * b = ui_button(parent, text, on ? UI_BLUE : 0x3A3A3C, cb, (void *)ud);
    lv_obj_set_height(b, 32);
    lv_obj_set_style_radius(b, 16, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(b, 0), &lv_font_montserrat_14, 0);
    return b;
}

static void build_locs(void)
{
    if(!s_locs_body) return;
    lv_obj_clean(s_locs_body);
    lv_obj_t * card = ui_section(s_locs_body, "MY LOCATIONS");
    for(int i = 0; i < s_nloc; i++) {
        sample_fill(i);
        lv_obj_t * row = ui_row(card, i == s_city ? LV_SYMBOL_OK : NULL, UI_BLUE, s_loc[i].name);
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, loc_row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        deg(row, s_wx[i].temp, &lv_font_montserrat_16, UI_DIM);
        if(s_nloc > 1) {
            lv_obj_t * d = ui_button(row, LV_SYMBOL_TRASH, 0x48484A, loc_del_cb, (void *)(intptr_t)i);
            lv_obj_set_size(d, 34, 30);
            lv_obj_set_style_pad_all(d, 0, 0);
        }
    }
    ui_nav_row(card, LV_SYMBOL_PLUS, UI_GREEN, "Add location", NULL, add_page_cb, NULL);

    int mode = sdk_get_int("wx.wmode", 0);
    card = ui_section(s_locs_body, "HOME SCREEN WIDGETS SHOW");
    lv_obj_t * row = ui_box(card);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(row, 10, 0);
    lv_obj_set_style_pad_gap(row, 8, 0);
    pill(row, "App's selection", mode == 0, wmode_cb, 0);
    pill(row, "One location", mode == 1, wmode_cb, 1);
    pill(row, "Swap all", mode == 2, wmode_cb, 2);
    if(mode == 1) {
        lv_obj_t * which = ui_box(card);
        lv_obj_set_size(which, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(which, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_style_pad_hor(which, 10, 0);
        lv_obj_set_style_pad_bottom(which, 10, 0);
        lv_obj_set_style_pad_gap(which, 8, 0);
        int cur = LV_CLAMP(0, sdk_get_int("wx.wloc", 0), s_nloc - 1);
        for(int i = 0; i < s_nloc; i++) pill(which, s_loc[i].name, i == cur, wloc_cb, i);
    }
    lv_obj_t * t = ui_label(s_locs_body, mode == 0 ? "Widgets follow the location selected in the app." :
                                         mode == 1 ? "Every weather widget stays on the location above." :
                                                     "Widgets swap to the next saved location every few seconds.",
                            &lv_font_montserrat_12, UI_DIM);
    lv_obj_set_width(t, LV_PCT(100));
    lv_label_set_long_mode(t, LV_LABEL_LONG_MODE_WRAP);
}

static void locs_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s_locs_body = ui_nav_push(s_root, "Locations", "Weather");
    lv_obj_add_event_cb(s_locs_body, locs_deleted_cb, LV_EVENT_DELETE, NULL);
    build_locs();
}

/*---- add by searching (open-meteo geocoding) ----*/

static void res_pick_cb(lv_event_t * e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if(i < 0 || i >= s_res_n) return;
    loc_add(s_res[i].name, s_res[i].lat, s_res[i].lon);
    s_city = s_nloc - 1;
    sdk_set_int("weather.city", s_city);
    fetch(s_city, app_reply, s_view);
    build_view();
    build_locs();
    ui_nav_pop(s_root);
    sdk_toast("Weather", "Location added.");
}

static void json_str(const char * p, char * out, size_t n)
{
    size_t k = 0;
    while(*p && *p != '"' && k < n - 1) {
        if(*p == '\\' && p[1] == 'u') {
            out[k++] = '?';
            p += 6;
            continue;
        }
        if((unsigned char)*p >= 0x80) { p++; continue; }
        out[k++] = *p++;
    }
    out[k] = '\0';
}

static void search_reply(bool ok, const char * body, void * ud)
{
    LV_UNUSED(ud);
    if(!s_results) return;
    lv_obj_clean(s_results);
    s_res_n = 0;
    const char * p = ok ? strstr(body, "\"results\"") : NULL;
    while(p && s_res_n < 6) {
        const char * nm = strstr(p, "\"name\":\"");
        if(!nm) break;
        nm += 8;
        const char * next = strstr(nm, "{\"id\":");
        double lat, lon;
        const char * la = strstr(nm, "\"latitude\":");
        const char * lo = strstr(nm, "\"longitude\":");
        if(!la || !lo || (next && (la > next || lo > next))) {
            p = next;
            continue;
        }
        sdk_json_num(la - 0, "latitude", &lat);
        sdk_json_num(lo - 0, "longitude", &lon);
        json_str(nm, s_res[s_res_n].name, sizeof(s_res[0].name));
        s_res[s_res_n].lat = lat;
        s_res[s_res_n].lon = lon;
        s_res[s_res_n].sub[0] = '\0';
        const char * co = strstr(nm, "\"country\":\"");
        if(co && (!next || co < next)) json_str(co + 11, s_res[s_res_n].sub, sizeof(s_res[0].sub));
        const char * ad = strstr(nm, "\"admin1\":\"");
        if(ad && (!next || ad < next)) {
            char reg[24], both[40];
            json_str(ad + 10, reg, sizeof(reg));
            lv_snprintf(both, sizeof(both), "%s, %s", reg, s_res[s_res_n].sub);
            lv_strncpy(s_res[s_res_n].sub, both, sizeof(s_res[0].sub) - 1);
        }
        s_res_n++;
        p = next;
    }
    lv_obj_t * card = ui_section(s_results, s_res_n ? "RESULTS" : NULL);
    if(!s_res_n) {
        ui_label(s_results, ok ? "No places found. Try another spelling." : "Could not search (offline?).",
                 &lv_font_montserrat_14, UI_DIM);
        return;
    }
    for(int i = 0; i < s_res_n; i++) {
        lv_obj_t * r = ui_nav_row(card, LV_SYMBOL_GPS, UI_BLUE, s_res[i].name, s_res[i].sub, res_pick_cb,
                                  (void *)(intptr_t)i);
        LV_UNUSED(r);
    }
}

static void search_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    const char * q = lv_textarea_get_text(s_search_ta);
    if(!q[0]) return;
    char enc[96];
    size_t n = 0;
    for(const char * c = q; *c && n < sizeof(enc) - 4; c++) {
        if((*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z')) enc[n++] = *c;
        else if(*c == ' ') enc[n++] = '+';
        else n += (size_t)lv_snprintf(enc + n, 4, "%%%02X", (unsigned char)*c);
    }
    enc[n] = '\0';
    char url[200];
    lv_snprintf(url, sizeof(url), "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=6&language=en&format=json",
                enc);
    lv_obj_clean(s_results);
    ui_label(s_results, "Searching...", &lv_font_montserrat_14, UI_DIM);
    ui_keyboard_hide(s_kb);
    sdk_http_get(url, search_reply, NULL, s_results);
}

static void kb_focus_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_keyboard_show(s_kb);
}

static void add_deleted_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s_results = s_search_ta = s_kb = NULL;
}

static void add_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "Add Location", "Locations");
    lv_obj_add_event_cb(body, add_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_obj_t * row = ui_box(body);
    lv_obj_set_size(row, LV_PCT(100), 40);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 8, 0);
    s_search_ta = lv_textarea_create(row);
    lv_textarea_set_one_line(s_search_ta, true);
    lv_textarea_set_placeholder_text(s_search_ta, "City name");
    lv_obj_set_flex_grow(s_search_ta, 1);
    lv_obj_set_height(s_search_ta, 38);
    lv_obj_set_style_radius(s_search_ta, 19, 0);
    lv_obj_set_style_bg_color(s_search_ta, lv_color_hex(0x2C2C2E), 0);
    lv_obj_set_style_text_color(s_search_ta, lv_color_white(), 0);
    lv_obj_set_style_border_width(s_search_ta, 0, 0);
    lv_obj_add_event_cb(s_search_ta, search_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(s_search_ta, kb_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(s_search_ta, kb_focus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * go = ui_button(row, LV_SYMBOL_RIGHT, UI_BLUE, search_cb, NULL);
    lv_obj_set_size(go, 44, 38);
    lv_obj_set_style_radius(go, 19, 0);
    s_results = ui_box(body);
    lv_obj_set_size(s_results, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_results, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_results, 8, 0);
    ui_label(s_results, "Type a city and press Enter.", &lv_font_montserrat_14, UI_DIM);
    s_kb = ui_keyboard(s_root, s_search_ta);
    ui_keyboard_show(s_kb);
}

static void dbg_open_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    const char * mode = getenv("ESPRING_LOCS");
    locs_page_cb(NULL);
    if(mode && mode[0] == 'a') {
        add_page_cb(NULL);
        if(getenv("ESPRING_Q")) {
            lv_textarea_set_text(s_search_ta, getenv("ESPRING_Q"));
            search_cb(NULL);
        }
    }
}

APP(app_weather, "Weather", LV_SYMBOL_TINT, 0x2F8FE0)
{
    s_root = root;
    s_locs_body = NULL;
    loc_load();
    lv_obj_set_style_pad_top(root, 38, 0);
    lv_obj_set_style_pad_hor(root, 14, 0);

    s_scroll = ui_box(root);
    lv_obj_set_size(s_scroll, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_bottom(s_scroll, OS_GESTURE_H + 8, 0);
    lv_obj_set_scrollable(s_scroll, true);
    lv_obj_set_clickable(s_scroll, true);
    lv_obj_set_scroll_dir(s_scroll, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_scroll, LV_SCROLLBAR_MODE_OFF);
    sdk_pull_refresh(s_scroll, refresh_cb, NULL);

    /* Rebuilt on every update; the scroller (with the refresh spinner) stays */
    s_view = ui_box(s_scroll);
    lv_obj_set_size(s_view, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(s_view, LV_PCT(101), 0);   /* always scrollable, so it can be pulled */
    lv_obj_set_flex_flow(s_view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_view, 12, 0);

    fetch(s_city, app_reply, s_view);
    build_view();
    if(getenv("ESPRING_LOCS")) lv_timer_set_repeat_count(lv_timer_create(dbg_open_cb, 900, root), 1);   /* simulator */
}

/*---------------- Widgets (small / medium / large) ----------------*/

static void widget_fill(lv_obj_t * box, int i, widget_size_t sz)
{
    if(!s_nloc) loc_load();
    sample_fill(i);
    const wx_t * w = &s_wx[i];
    const char * text, * sym;
    uint32_t top, bottom;
    condition(w->code, &text, &sym, &top, &bottom);

    lv_obj_clean(box);
    lv_obj_set_style_bg_color(box, lv_color_hex(top), 0);
    lv_obj_set_style_bg_grad_color(box, lv_color_hex(bottom), 0);
    lv_obj_set_style_bg_grad_dir(box, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_90, 0);
    lv_obj_update_layout(box);
    int32_t cw = lv_obj_get_content_width(box), ch = lv_obj_get_content_height(box);

    if(sz == WIDGET_SMALL) {
        lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(box, 2, 0);
        ui_label(box, s_loc[i].name, &lv_font_montserrat_14, 0xFFFFFF);
        deg(box, w->temp, &lv_font_montserrat_36, 0xFFFFFF);
        lv_obj_t * spacer = ui_box(box);
        lv_obj_set_flex_grow(spacer, 1);
        ui_label(box, sym, &lv_font_montserrat_16, 0xFFFFFF);
        ui_label(box, text, &lv_font_montserrat_12, 0xFFFFFF);
    }
    else if(sz == WIDGET_MEDIUM) {
        lv_obj_t * l = ui_label(box, s_loc[i].name, &lv_font_montserrat_14, 0xFFFFFF);
        lv_obj_align(l, LV_ALIGN_TOP_LEFT, 0, 0);
        l = deg(box, w->temp, &lv_font_montserrat_36, 0xFFFFFF);
        lv_obj_align(l, LV_ALIGN_TOP_LEFT, 0, 18);
        l = ui_label(box, "", &lv_font_montserrat_12, 0xFFFFFF);
        lv_label_set_text_fmt(l, "%s %s\nH:%d\xC2\xB0 L:%d\xC2\xB0", sym, text, w->hi[0], w->lo[0]);
        lv_obj_align(l, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_t * days = ui_box(box);
        lv_obj_set_size(days, LV_MIN(150, cw - 100), ch);
        lv_obj_align(days, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_flex_flow(days, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(days, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        for(int d = 1; d < DAYS; d++) {
            lv_obj_t * col = ui_box(days);
            lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_row(col, 4, 0);
            lv_obj_t * dl = ui_label(col, "", &lv_font_montserrat_12, 0xE0E8F0);
            lv_label_set_text_fmt(dl, "+%dd", d);
            deg(col, w->hi[d], &lv_font_montserrat_14, 0xFFFFFF);
            deg(col, w->lo[d], &lv_font_montserrat_12, 0xD0D8E0);
        }
    }
    else {
        lv_obj_t * l = ui_label(box, s_loc[i].name, &lv_font_montserrat_18, 0xFFFFFF);
        lv_obj_align(l, LV_ALIGN_TOP_LEFT, 0, 0);
        l = deg(box, w->temp, &lv_font_montserrat_48, 0xFFFFFF);
        lv_obj_align(l, LV_ALIGN_TOP_LEFT, 0, 24);
        l = ui_label(box, "", &lv_font_montserrat_14, 0xFFFFFF);
        lv_label_set_text_fmt(l, "%s %s\nH:%d\xC2\xB0 L:%d\xC2\xB0", sym, text, w->hi[0], w->lo[0]);
        lv_obj_align(l, LV_ALIGN_TOP_RIGHT, 0, 6);
        /* hourly strip */
        lv_obj_t * hr = ui_box(box);
        lv_obj_set_size(hr, cw, 56);
        lv_obj_set_pos(hr, 0, 92);
        lv_obj_set_flex_flow(hr, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(hr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        for(int h = 0; h < 6; h++) {
            lv_obj_t * col = ui_box(hr);
            lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_t * t = lv_label_create(col);
            if(h == 0) lv_label_set_text(t, "Now");
            else lv_label_set_text_fmt(t, "+%dh", h * 2);
            lv_obj_set_style_text_color(t, lv_color_white(), 0);
            lv_obj_set_style_text_font(t, &lv_font_montserrat_12, 0);
            deg(col, w->hourly[LV_MIN(h * 2, HOURS - 1)], &lv_font_montserrat_16, 0xFFFFFF);
        }
        /* daily rows */
        int32_t top_y = 160;
        int rows = LV_CLAMP(2, (ch - top_y) / 30, DAYS);
        for(int d = 0; d < rows; d++) {
            lv_obj_t * r = ui_box(box);
            lv_obj_set_size(r, cw, 26);
            lv_obj_set_pos(r, 0, top_y + d * 30);
            lv_obj_set_style_radius(r, 8, 0);
            lv_obj_set_style_bg_opa(r, LV_OPA_20, 0);
            lv_obj_set_style_bg_color(r, lv_color_black(), 0);
            lv_obj_t * a = ui_label(r, d == 0 ? "Today" : "", &lv_font_montserrat_14, 0xFFFFFF);
            if(d) lv_label_set_text_fmt(a, "+%dd", d);
            lv_obj_align(a, LV_ALIGN_LEFT_MID, 8, 0);
            lv_obj_t * rng = ui_label(r, "", &lv_font_montserrat_14, 0xFFFFFF);
            lv_label_set_text_fmt(rng, "%d\xC2\xB0   %d\xC2\xB0", w->lo[d], w->hi[d]);
            lv_obj_align(rng, LV_ALIGN_RIGHT_MID, -8, 0);
        }
    }
}

typedef struct {
    lv_obj_t * box;
    int shown;
} wst_t;

static void widget_tick(lv_timer_t * t)
{
    lv_obj_t * box = lv_timer_get_user_data(t);
    int i = widget_loc();
    widget_size_t sz = (widget_size_t)(intptr_t)lv_obj_get_user_data(box);
    int prev = (int)(intptr_t)lv_obj_get_user_data(lv_obj_get_parent(box)) - 1;
    fetch(i, on_reply, box);     /* tick redraws once the reply lands */
    widget_fill(box, i, sz);
    if(prev >= 0 && prev != i) {                         /* swapped to another location: fade the new one in */
        for(uint32_t c = 0; c < lv_obj_get_child_count(box); c++) {
            lv_obj_t * ch = lv_obj_get_child(box, (int32_t)c);
            fx_set(ch, FX_OPA, 0);
            fx_to(ch, FX_OPA, 255, 0.35f, 1.0f, c * 20, NULL);
        }
    }
    lv_obj_set_user_data(lv_obj_get_parent(box), (void *)(intptr_t)(i + 1));
}

static void weather_widget(lv_obj_t * box)
{
    widget_size_t sz = sdk_widget_size();
    lv_obj_set_user_data(box, (void *)(intptr_t)sz);
    widget_tick(sdk_timer(box, widget_tick, 3000, box));
}

static void lock_tick(lv_timer_t * t)
{
    lv_obj_t * slot = lv_timer_get_user_data(t);
    if(!s_nloc) loc_load();
    int i = widget_loc();
    const char * text, * sym;
    uint32_t top, bottom;
    sample_fill(i);
    condition(s_wx[i].code, &text, &sym, &top, &bottom);
    lv_label_set_text_fmt(lv_obj_get_child(slot, 0), "%s  %d\xC2\xB0", sym, s_wx[i].temp);
    lv_label_set_text(lv_obj_get_child(slot, 1), s_loc[i].name);
}

static void widget_lock(lv_obj_t * slot)
{
    lv_obj_set_flex_flow(slot, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(slot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    ui_label(slot, "", &lv_font_montserrat_16, 0xFFFFFF);
    ui_label(slot, "", &lv_font_montserrat_12, 0xE5E5EA);
    lock_tick(sdk_timer(slot, lock_tick, 5000, slot));
}

/* Both registry entries are resizable and share the renderer, so saved layouts keep working */
const widget_desc_t widget_weather = { "Weather", &app_weather, WIDGET_SMALL, weather_widget, widget_lock, true };
const widget_desc_t widget_weather_wide = { "Weather Forecast", &app_weather, WIDGET_MEDIUM, weather_widget, NULL, true };
