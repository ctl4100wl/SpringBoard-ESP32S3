/**
 * @file apps.c
 * App registry. Add an app: write src/apps/app_<name>.c exporting an app_desc_t and list it here.
 * Default home layout: everything goes on the grid except the last HOME_DOCK entries (the dock).
 */
#include "../os/sdk.h"

extern const app_desc_t app_settings;
extern const app_desc_t app_calculator;
extern const app_desc_t app_clock;
extern const app_desc_t app_calendar;
extern const app_desc_t app_weather;
extern const app_desc_t app_reminders;
extern const app_desc_t app_sketch;
extern const app_desc_t app_2048;
extern const app_desc_t app_notify;
extern const app_desc_t app_phone;
extern const app_desc_t app_notes;
extern const app_desc_t app_music;
extern const app_desc_t app_photos;
extern const app_desc_t app_sdktest;
extern const app_desc_t app_browser;
extern const app_desc_t app_light;
extern const app_desc_t app_store;
extern const app_desc_t app_video;
extern const app_desc_t app_doom;

const app_desc_t * const os_apps[] = {
    &app_settings,
    &app_calculator,
    &app_clock,
    &app_calendar,
    &app_weather,
    &app_reminders,
    &app_sketch,
    &app_2048,
    &app_notify,
    /* dock */
    &app_phone,
    &app_notes,
    &app_music,
    &app_photos,
    /* new apps go after the dock entries: saved layouts refer to apps by index */
    &app_sdktest,
    &app_browser,
    &app_light,
    &app_store,
    &app_video,
    &app_doom,
};

const int os_app_count = (int)(sizeof(os_apps) / sizeof(os_apps[0]));
const int os_dock_start = 9;       /* default dock = os_apps[os_dock_start .. +HOME_DOCK-1], the rest fills the grid */

/* Widget gallery (jiggle mode "+"). The first two start on home page 2. */
extern const widget_desc_t widget_clock;
extern const widget_desc_t widget_weather;
extern const widget_desc_t widget_weather_wide;
extern const widget_desc_t widget_photo;
extern const widget_desc_t widget_reminders;
extern const widget_desc_t widget_battery;
extern const widget_desc_t widget_calendar;
extern const widget_desc_t widget_music;
extern const widget_desc_t widget_notes;
extern const widget_desc_t widget_steps;
extern const widget_desc_t widget_sdktest;
extern const widget_desc_t widget_rings;
extern const widget_desc_t widget_daylight;

const widget_desc_t * const os_widgets[] = {
    &widget_clock, &widget_weather, &widget_weather_wide, &widget_photo, &widget_reminders,
    &widget_battery, &widget_calendar, &widget_music, &widget_notes, &widget_steps, &widget_sdktest,
    &widget_rings, &widget_daylight,
};
const int os_widget_count = (int)(sizeof(os_widgets) / sizeof(os_widgets[0]));
