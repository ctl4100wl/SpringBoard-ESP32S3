/**
 * @file app_notify.c
 * Notification lab: fire iOS-style notifications from different apps, now or after a delay
 * (lock the screen from Control Center first to see them stack on the Lock Screen).
 */
#include "../os/sdk.h"

extern const app_desc_t app_phone, app_calendar, app_weather, app_music, app_reminders, app_notify;

typedef struct {
    const app_desc_t * app;
    const char * title;
    const char * body;
} preset_t;

static const preset_t k_presets[] = {
    { &app_phone,     "Mom",            "Are you coming for dinner tonight? I'm making pho." },
    { &app_calendar,  "Design Review",  "Starts in 10 minutes - Room 4B" },
    { &app_weather,   "Rain Expected",  "Light rain starting around 5 PM. Bring an umbrella." },
    { &app_music,     "New Release",    "Your favourite artist just dropped a new album." },
    { &app_reminders, "Water the plants", "Today" },
};
#define PRESET_N ((int)(sizeof(k_presets) / sizeof(k_presets[0])))

static lv_obj_t * s_count;
static int s_next;

static void count_refresh(void)
{
    if(s_count) lv_label_set_text_fmt(s_count, "%d in Notification Center", notify_count());
}

static void fire(int i)
{
    const preset_t * p = &k_presets[i % PRESET_N];
    sdk_notify(p->app, p->title, p->body);
    count_refresh();
}

static void preset_cb(lv_event_t * e)
{
    fire((int)(intptr_t)lv_event_get_user_data(e));
}

static void delayed_cb(lv_timer_t * t)
{
    fire(s_next++);
    lv_timer_delete(t);
}

static void later_cb(lv_event_t * e)
{
    /* Not tied to the app window: it still fires after you leave the app or lock the screen */
    lv_timer_create(delayed_cb, (uint32_t)(intptr_t)lv_event_get_user_data(e), NULL);
    sdk_toast("Scheduled", "You'll receive a notification shortly.");
}

static void burst_tick(lv_timer_t * t)
{
    LV_UNUSED(t);
    fire(s_next++);
}

static void ta_cb(lv_event_t * e)
{
    lv_obj_set_hidden(lv_event_get_user_data(e), false);
}

static void burst_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_timer_t * t = lv_timer_create(burst_tick, 700, NULL);
    lv_timer_set_repeat_count(t, 3);
}

static void custom_cb(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_user_data(e);
    const char * txt = lv_textarea_get_text(ta);
    sdk_notify(&app_notify, "Notif Test", txt[0] ? txt : "Test notification");
    count_refresh();
}

static void clear_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    notify_clear();
    count_refresh();
}

static void tick(lv_timer_t * t)
{
    LV_UNUSED(t);
    count_refresh();
}

static void root_deleted(lv_event_t * e)
{
    LV_UNUSED(e);
    s_count = NULL;
}

APP(app_notify, "Notif Test", LV_SYMBOL_BELL, 0xFF3B30)
{
    lv_obj_t * body = ui_app_scaffold(root, "Notif Test");
    lv_obj_add_event_cb(root, root_deleted, LV_EVENT_DELETE, NULL);

    s_count = ui_label(body, "", &lv_font_montserrat_14, UI_DIM);
    sdk_timer(root, tick, 1000, NULL);
    count_refresh();

    lv_obj_t * card = ui_section(body, "SEND NOW");
    for(int i = 0; i < PRESET_N; i++) {
        const preset_t * p = &k_presets[i];
        ui_nav_row(card, p->app->symbol, p->app->color, p->title, NULL, preset_cb, (void *)(intptr_t)i);
    }

    card = ui_section(body, "TIMING");
    ui_nav_row(card, LV_SYMBOL_LOOP, UI_ORANGE, "Send in 5 seconds", NULL, later_cb, (void *)(intptr_t)5000);
    ui_nav_row(card, LV_SYMBOL_LOOP, UI_ORANGE, "Send in 15 seconds", NULL, later_cb, (void *)(intptr_t)15000);
    ui_nav_row(card, LV_SYMBOL_LIST, 0x5E5CE6, "Burst of 3", NULL, burst_cb, NULL);

    card = ui_section(body, "CUSTOM");
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_t * ta = lv_textarea_create(card);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "Message text...");
    lv_obj_t * send = ui_button(card, "Send", UI_BLUE, custom_cb, ta);
    lv_obj_set_width(send, LV_PCT(100));
    lv_obj_t * kb = ui_keyboard(root, ta);
    lv_obj_add_event_cb(ta, ta_cb, LV_EVENT_CLICKED, kb);

    card = ui_section(body, NULL);
    ui_nav_row(card, LV_SYMBOL_TRASH, UI_RED, "Clear Notification Center", NULL, clear_cb, NULL);

    ui_label(body, "Notifications appear on the Lock Screen and in Notification Center.",
             &lv_font_montserrat_12, UI_DIM);
}
