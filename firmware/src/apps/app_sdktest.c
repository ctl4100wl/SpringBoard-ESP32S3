/**
 * @file app_sdktest.c
 * SDK Test: one app that touches every part of the app SDK (see src/apps/README.md) and hosts the
 * Liquid Glass lab. Read it as a cookbook: each section is a few lines.
 *
 *   ui_app_scaffold / ui_section / ui_row / ui_switch_row / ui_nav_row / ui_nav_push   layout + navigation
 *   sdk_get_int / sdk_set_int                                                          persisted storage
 *   sdk_timer                                                                          timers bound to the app window
 *   sdk_notify / sdk_toast                                                             banners + Notification Center
 *   sdk_http_get + sdk_json_num                                                        network
 *   sdk_pull_refresh                                                                   pull to refresh
 *   sdk_pick_image / sdk_image_load                                                    images from the PC
 *   fx_to / fx_kick                                                                    spring physics
 *   ui_lens                                                                            refractive glass
 */
#include <stdio.h>
#include <stdlib.h>
#include "../os/sdk.h"

extern const app_desc_t app_sdktest;

static lv_obj_t * s_launch_lbl, * s_up_lbl, * s_net_lbl, * s_img_box, * s_ball, * s_arena;
static lv_draw_buf_t * s_img;
static uint32_t s_t0;

/*---------------- storage ----------------*/

static void launches_refresh(void)
{
    lv_label_set_text_fmt(s_launch_lbl, "%d", sdk_get_int("sdktest.launches", 0));
}

static void reset_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    sdk_set_int("sdktest.launches", 0);
    launches_refresh();
}

/*---------------- timers ----------------*/

static void uptime_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    lv_label_set_text_fmt(s_up_lbl, "%us", (unsigned)(lv_tick_elaps(s_t0) / 1000));
}

/*---------------- notifications ----------------*/

static void banner_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    sdk_notify(&app_sdktest, "SDK Test", "Your notification is ready.");
}

static void toast_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    sdk_toast("Toast", "sdk_toast() posts a system notification with no app.");
}

/*---------------- network ----------------*/

static void net_reply(bool ok, const char * body, void * ud)
{
    LV_UNUSED(ud);
    double t;
    if(ok && sdk_json_num(body, "temperature", &t)) lv_label_set_text_fmt(s_net_lbl, "%d\xC2\xB0" "C", (int)t);
    else lv_label_set_text(s_net_lbl, "offline");
}

static void net_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_label_set_text(s_net_lbl, "...");
    sdk_http_get("https://api.open-meteo.com/v1/forecast?latitude=37.33&longitude=-122.03&current_weather=true",
                 net_reply, NULL, s_net_lbl);
}

/*---------------- physics ----------------*/

static void arena_cb(lv_event_t * e)
{
    lv_point_t p;
    lv_indev_get_point(lv_indev_active(), &p);
    lv_area_t a;
    lv_obj_get_coords(s_arena, &a);
    float x = (float)(p.x - a.x1 - 20), y = (float)(p.y - a.y1 - 20);
    x = LV_CLAMP(0.0f, x, (float)(lv_area_get_width(&a) - 40));
    y = LV_CLAMP(0.0f, y, (float)(lv_area_get_height(&a) - 40));
    fx_to(s_ball, FX_X, x - (float)lv_obj_get_x(s_ball), FX_BOUNCY, 0, NULL);
    fx_to(s_ball, FX_Y, y - (float)lv_obj_get_y(s_ball), FX_BOUNCY, 0, NULL);
    LV_UNUSED(e);
}

/*---------------- images ----------------*/

static void pick_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    char path[300];
    if(!sdk_pick_image(path, sizeof(path))) {
        sdk_toast("SDK Test", "No image selected.");
        return;
    }
    lv_draw_buf_t * b = sdk_image_load(path, 96, 54);
    if(!b) {
        sdk_toast("SDK Test", "This image couldn't be opened.");
        return;
    }
    if(s_img) lv_draw_buf_destroy(s_img);
    s_img = b;
    lv_obj_clean(s_img_box);
    lv_image_set_src(lv_image_create(s_img_box), s_img);
}

/*---------------- pull to refresh ----------------*/

static void refreshed_cb(lv_timer_t * t)
{
    sdk_pull_refresh_done(lv_timer_get_user_data(t));
    lv_timer_delete(t);
    sdk_toast("SDK Test", "Refreshed.");
}

static void refresh_cb(lv_obj_t * body, void * ud)
{
    LV_UNUSED(ud);
    lv_timer_create(refreshed_cb, 900, body);
}

/*---------------- Liquid Glass lab ----------------*/

static lv_obj_t * s_lens[2];
static int32_t s_bezel = 22, s_strength = 85;
static bool s_chroma = true;

static void lens_apply(void)
{
    ui_lens_settings_set(s_bezel, s_strength, s_chroma);
    for(int i = 0; i < 2; i++) if(s_lens[i]) ui_lens(s_lens[i], s_bezel, s_strength, s_chroma);
}

static void lens_drag_cb(lv_event_t * e)
{
    lv_obj_t * o = lv_event_get_current_target_obj(e);
    lv_point_t v;
    lv_indev_get_vect(lv_indev_active(), &v);
    lv_obj_t * scene = lv_obj_get_parent(o);
    int32_t x = LV_CLAMP(0, lv_obj_get_x(o) + v.x, lv_obj_get_width(scene) - lv_obj_get_width(o));
    int32_t y = LV_CLAMP(0, lv_obj_get_y(o) + v.y, lv_obj_get_height(scene) - lv_obj_get_height(o));
    lv_obj_set_pos(o, x, y);
}

static void bezel_cb(lv_event_t * e)
{
    s_bezel = lv_slider_get_value(lv_event_get_target_obj(e));
    lens_apply();
}

static void strength_cb(lv_event_t * e)
{
    s_strength = lv_slider_get_value(lv_event_get_target_obj(e));
    lens_apply();
}

static void chroma_cb(lv_event_t * e)
{
    s_chroma = lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED);
    lens_apply();
}

static void lens_slider(lv_obj_t * parent, const char * name, int32_t min, int32_t max, int32_t val, lv_event_cb_t cb)
{
    lv_obj_t * row = ui_box(parent);
    lv_obj_set_size(row, LV_PCT(100), 28);
    lv_obj_t * l = ui_label(row, name, &lv_font_montserrat_14, UI_DIM);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t * s = lv_slider_create(row);
    lv_obj_set_size(s, 170, 8);
    lv_obj_align(s, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_slider_set_range(s, min, max);
    lv_slider_set_value(s, val, LV_ANIM_OFF);
    lv_obj_add_event_cb(s, cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void lab_deleted_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    os_save();
    s_lens[0] = s_lens[1] = NULL;
}

static void lab_open(lv_obj_t * root)
{
    ui_lens_settings_get(&s_bezel, &s_strength, &s_chroma);
    lv_obj_t * body = ui_nav_push(root, "Liquid Glass", "SDK Test");
    lv_obj_set_scrollable(body, false);
    lv_obj_add_event_cb(body, lab_deleted_cb, LV_EVENT_DELETE, NULL);

    /* A busy scene to refract: gradient, circles and stripes */
    lv_obj_t * scene = ui_box(body);
    lv_obj_set_size(scene, LV_PCT(100), 300);
    lv_obj_set_style_radius(scene, 24, 0);
    lv_obj_set_style_clip_corner(scene, true, 0);
    lv_obj_set_style_bg_opa(scene, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(scene, lv_color_hex(0xFF5E62), 0);
    lv_obj_set_style_bg_grad_color(scene, lv_color_hex(0x2840C8), 0);
    lv_obj_set_style_bg_grad_dir(scene, LV_GRAD_DIR_VER, 0);
    static const uint32_t cols[] = { 0xFFD60A, 0x30D158, 0xFFFFFF, 0x64D2FF, 0xFF375F, 0xBF5AF2 };
    for(int i = 0; i < 14; i++) {
        lv_obj_t * c = ui_box(scene);
        int32_t d = 34 + (i * 17) % 46;
        lv_obj_set_size(c, d, d);
        lv_obj_set_pos(c, (i * 53) % 250, (i * 71) % 250);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(c, lv_color_hex(cols[i % 6]), 0);
    }
    for(int i = 0; i < 6; i++) {
        lv_obj_t * s = ui_box(scene);
        lv_obj_set_size(s, LV_PCT(100), 3);
        lv_obj_set_pos(s, 0, 20 + i * 52);
        lv_obj_set_style_bg_opa(s, LV_OPA_50, 0);
        lv_obj_set_style_bg_color(s, lv_color_white(), 0);
    }
    lv_obj_t * t = ui_label(scene, "REFRACT", &lv_font_montserrat_48, 0xFFFFFF);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t * pill = ui_box(scene);
    lv_obj_set_size(pill, 170, 72);
    lv_obj_set_pos(pill, 20, 40);
    lv_obj_set_style_radius(pill, 36, 0);
    s_lens[0] = pill;
    lv_obj_t * disc = ui_box(scene);
    lv_obj_set_size(disc, 110, 110);
    lv_obj_set_pos(disc, 150, 170);
    lv_obj_set_style_radius(disc, LV_RADIUS_CIRCLE, 0);
    s_lens[1] = disc;
    for(int i = 0; i < 2; i++) {
        lv_obj_set_clickable(s_lens[i], true);
        lv_obj_add_event_cb(s_lens[i], lens_drag_cb, LV_EVENT_PRESSING, NULL);
    }
    lens_apply();

    lens_slider(body, "Bezel", 6, 40, s_bezel, bezel_cb);
    lens_slider(body, "Glass thickness", 0, 100, s_strength, strength_cb);
    lv_obj_t * row = ui_box(body);
    lv_obj_set_size(row, LV_PCT(100), 30);
    lv_obj_align(ui_label(row, "Chromatic fringe", &lv_font_montserrat_14, UI_DIM), LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t * sw = lv_switch_create(row);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
    if(s_chroma) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, chroma_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t * hint = ui_label(body, "Drag the glass over the scene", &lv_font_montserrat_12, UI_DIM);
    LV_UNUSED(hint);
}

static void lab_timer_cb(lv_timer_t * t)
{
    lab_open(lv_timer_get_user_data(t));
}

static void lab_open_cb(lv_event_t * e)
{
    lab_open(lv_event_get_user_data(e));
}

/*---------------- the app ----------------*/

static lv_obj_t * action_row(lv_obj_t * card, const char * sym, uint32_t color, const char * text, const char * btn,
                             lv_event_cb_t cb)
{
    lv_obj_t * row = ui_row(card, sym, color, text);
    lv_obj_t * b = ui_button(row, btn, UI_BLUE, cb, NULL);
    lv_obj_set_height(b, 30);
    lv_obj_set_style_pad_hor(b, 12, 0);
    return row;
}

APP(app_sdktest, "SDK Test", LV_SYMBOL_LIST, 0x5E5CE6)
{
    s_t0 = lv_tick_get();
    s_img = NULL;
    int n = sdk_get_int("sdktest.launches", 0) + 1;
    sdk_set_int("sdktest.launches", n);

    lv_obj_t * body = ui_app_scaffold(root, "SDK Test");
    sdk_pull_refresh(body, refresh_cb, NULL);

    lv_obj_t * card = ui_section(body, "STORAGE");
    lv_obj_t * row = action_row(card, LV_SYMBOL_SAVE, UI_BLUE, "Launches", "Reset", reset_cb);
    s_launch_lbl = ui_label(row, "", &lv_font_montserrat_16, UI_DIM);
    lv_obj_move_to_index(s_launch_lbl, 2);
    launches_refresh();

    card = ui_section(body, "TIMERS");
    row = ui_row(card, LV_SYMBOL_REFRESH, UI_ORANGE, "Uptime");
    s_up_lbl = ui_label(row, "0s", &lv_font_montserrat_16, UI_DIM);
    sdk_timer(root, uptime_cb, 1000, NULL);

    card = ui_section(body, "NOTIFICATIONS");
    action_row(card, LV_SYMBOL_BELL, UI_RED, "Banner from this app", "Send", banner_cb);
    action_row(card, LV_SYMBOL_WARNING, UI_ORANGE, "System toast", "Send", toast_cb);

    card = ui_section(body, "NETWORK");
    row = action_row(card, LV_SYMBOL_WIFI, UI_GREEN, "Temperature", "Fetch", net_cb);
    s_net_lbl = ui_label(row, "-", &lv_font_montserrat_16, UI_DIM);
    lv_obj_move_to_index(s_net_lbl, 2);

    card = ui_section(body, "MOTION");
    s_arena = ui_box(card);
    lv_obj_set_size(s_arena, LV_PCT(100), 110);
    lv_obj_set_clickable(s_arena, true);
    lv_obj_add_event_cb(s_arena, arena_cb, LV_EVENT_CLICKED, NULL);
    s_ball = ui_box(s_arena);
    lv_obj_set_size(s_ball, 40, 40);
    lv_obj_set_pos(s_ball, 20, 35);
    lv_obj_set_style_radius(s_ball, LV_RADIUS_CIRCLE, 0);
    ui_icon_style(s_ball, 0x5E5CE6);

    card = ui_section(body, "PHOTOS");
    row = action_row(card, LV_SYMBOL_IMAGE, UI_BLUE, "Pick from PC", "Browse", pick_cb);
    s_img_box = ui_box(row);
    lv_obj_set_size(s_img_box, 96, 54);
    lv_obj_move_to_index(s_img_box, 2);
    LV_UNUSED(row);

    card = ui_section(body, "LIQUID GLASS");
    lv_obj_t * nav = ui_nav_row(card, LV_SYMBOL_EYE_OPEN, UI_BLUE, "Liquid glass lab", "", lab_open_cb, root);
    LV_UNUSED(nav);
    if(getenv("ESPRING_LAB")) lv_timer_set_repeat_count(lv_timer_create(lab_timer_cb, 900, root), 1);   /* simulator: jump to the lab */
}

/*---------------- widget ----------------*/

WIDGET(widget_sdktest, "Glass Lab", &app_sdktest, WIDGET_MEDIUM, NULL)
{
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xFF5E62), 0);
    lv_obj_set_style_bg_grad_color(box, lv_color_hex(0x2840C8), 0);
    lv_obj_set_style_bg_grad_dir(box, LV_GRAD_DIR_HOR, 0);
    static const uint32_t cols[] = { 0xFFD60A, 0x30D158, 0xFFFFFF, 0x64D2FF };
    for(int i = 0; i < 6; i++) {
        lv_obj_t * c = ui_box(box);
        lv_obj_set_size(c, 28 + (i % 3) * 8, 28 + (i % 3) * 8);
        lv_obj_set_pos(c, 10 + i * 46, 10 + (i % 2) * 36);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(c, lv_color_hex(cols[i % 4]), 0);
    }
    lv_obj_t * g = ui_box(box);
    lv_obj_set_size(g, 150, 54);
    lv_obj_align(g, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(g, 27, 0);
    ui_lens(g, 16, 85, true);
    lv_obj_center(ui_label(g, "Liquid Glass", &lv_font_montserrat_16, 0xFFFFFF));
}
