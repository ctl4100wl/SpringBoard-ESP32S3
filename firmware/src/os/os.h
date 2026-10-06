/**
 * @file os.h
 * Shared interface between the shell (window manager), the home screen and the apps.
 */
#ifndef OS_H
#define OS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "lvgl/lvgl.h"
#include "fx.h"

/*********************
 *      DEFINES
 *********************/

#define OS_MAX_APPS         24
#define OS_GESTURE_H        22      /* invisible bottom strip that owns the system swipe */
#define OS_WALLPAPER_COUNT  6
#define OS_WALL_CUSTOM      0xFF    /* wallpaper index meaning "image chosen from the PC" */
#define OS_LOCK_STYLES      6
#define OS_LOCK_COLOR_AUTO  8       /* lock_color value: pick the clock colour from the wallpaper */
#define OS_NAME             "S3-Springboard"

#define HOME_PAGES          2
#define HOME_COLS           4
#define HOME_ROWS           4
#define HOME_DOCK           4
#define HOME_SLOTS          (HOME_COLS * HOME_ROWS)
#define HOME_LOC_DOCK0      (HOME_PAGES * HOME_SLOTS)
#define HOME_LOC_COUNT      (HOME_LOC_DOCK0 + HOME_DOCK)
#define HOME_WIDGETS        6       /* placed widget instances */
#define HOME_FOLDER_ITEMS   8

/**********************
 *      TYPEDEFS
 **********************/

/** One app = one file in src/apps. `create` builds the UI inside a full-screen root.
 *  The root is deleted when the app leaves; free timers etc. on LV_EVENT_DELETE of the root.
 *  Keep app state in statics so it survives relaunches. */
typedef struct {
    const char * name;
    const char * symbol;
    uint32_t color;
    void (*create)(lv_obj_t * root);
} app_desc_t;

typedef struct {
    bool wifi;
    bool bluetooth;
    bool airplane;
    uint8_t brightness;     /* 10..100 */
    uint8_t wallpaper;      /* 0..OS_WALLPAPER_COUNT-1 */
    uint8_t volume;         /* 0..100 */
    bool dnd;               /* Do Not Disturb: silences sdk_toast */
    bool flashlight;        /* not persisted */
    bool cellular;
    bool pill;              /* show the home indicator in apps */
    bool quick_switch;      /* swipe sideways on the bottom edge to change apps */
    bool previews;          /* notification text on the lock screen */
    uint8_t lock_style;     /* clock preset 0..OS_LOCK_STYLES-1 */
    uint8_t lock_color;     /* clock tint index */
    uint8_t lock_wall;      /* lock wallpaper, OS_WALL_CUSTOM = image */
    uint8_t about_style;    /* 0 iOS, 1 HyperOS, 2 Hybrid */
    bool edge_back;         /* swipe in from the left edge = back */
    uint8_t edge_size;      /* width of that edge zone in px */
    int8_t lock_widget[4];  /* two-column lock screen grid, -1 = empty */
    uint8_t lock_widget_span[4]; /* 1 = compact, 2 = full row */
    bool low_power;         /* Clock icon second hand ticks once per second */
} os_settings_t;

/** Widget placed on the home screen (see widget_desc_t in sdk.h) */
typedef struct {
    int8_t widget;          /* index into os_widgets, -1 = free */
    int8_t page;
    int8_t cell;            /* top-left grid slot */
    int8_t size;            /* 0 = the widget's own size, else widget_size_t + 1 */
} home_widget_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/* Registry (apps.c) */
extern const app_desc_t * const os_apps[];
extern const int os_app_count;
extern const int os_dock_start;                     /* index of the first default dock app */

/* Settings + persistence (persist.c) */
extern os_settings_t os_settings;
void os_load(void);
void os_save(void);

/* Shell (shell.c) */
void shell_init(void);
void shell_settings_apply(void);
void shell_wallpaper_colors(int idx, uint32_t * top, uint32_t * bottom);
void shell_statusbar_set_visible(bool visible);
void shell_statusbar_raise(void);
bool shell_home_idle(void);                         /* home screen owns input */
void shell_launch_from_home(int id, const lv_area_t * icon_area);
void shell_launch_from_widget(int id, lv_obj_t * widget_box);  /* morphs back into the widget on exit */
void shell_open_app(int id);                        /* from anywhere; exits with a fade */
void shell_open_app_from(int id, const lv_area_t * from);   /* morph in from `from`, fade out on exit */
void shell_lock_changed(bool visible);
void shell_clock_shade_progress(float progress);
void shell_go_home_instant(void);                   /* close any app / recents without animation */
void shell_recents_open(void);                       /* Home Screen button and gesture */
lv_draw_buf_t * shell_wallpaper_image(bool lock);   /* custom image or NULL */
void shell_wallpaper_colors_lock(int idx, uint32_t * top, uint32_t * bottom);

/* iOS-style status icons (statusbar.c) */
lv_obj_t * statusbar_icons_create(lv_obj_t * parent);
void statusbar_icons_update(void);
void statusbar_icons_set_color(lv_color_t c);       /* adaptive status bar colour */

/* Lock screen + notification center (lock.c) */
void lock_init(void);
void lock_now(void);
bool lock_visible(void);
void lock_refresh(void);                            /* settings changed */
void notify_post(int app_id, const char * title, const char * body);
int notify_count(void);
void notify_clear(void);
void lock_debug_unlock(void);                       /* simulator test hooks */
void lock_debug_shade(float p);                     /* freeze the pull-down shade at progress 0..1 */

/* Image import (import.c): drop a file on the simulator window, pick where to use it */
void import_init(void);
void import_offer(const char * path);               /* glass sheet: Home / Lock / Both */
bool os_drop_poll(char * path_out, size_t n);       /* hal.c: next dropped file, false if none */

/* Control Center (control_center.c) */
void cc_init(void);
void cc_set_enabled(bool en);
bool cc_visible(void);
void cc_close(void);
void cc_debug_pose(float p);
void cc_debug_pop(void);                          /* simulator test hook */

/* Home screen (home.c) */
void home_create(lv_obj_t * parent);
bool home_get_icon_area(int id, lv_area_t * out);   /* false if not visible on the current page */
lv_obj_t * home_get_icon_object(int id);
void home_set_icon_hidden(int id, bool hidden);
void home_edit_enter(void);
void home_debug_resize(int k, int size);            /* simulator test hooks */
void home_debug_wres(int k, int axis, int dx, int dy);
void home_debug_wdrag(int k, int x, int y);
void home_debug_wdrop(void);
void home_debug_cmd(const char * cmd);
bool notify_banner_active(void);        /* a notification banner is on screen */
void shell_debug_home(void);
int home_widget_count(int widget);                  /* placed instances of os_widgets[widget] */
bool home_widget_add(int widget);                   /* false = no room or no free instance */
bool home_widget_remove(int widget);                /* removes one instance */
int home_widget_free_slots(void);
void home_card_sync(void);                          /* wallpaper changed while the edit card is up */
void home_edit_exit(void);
bool home_in_edit(void);
void home_reset_layout(void);
void home_layout_export(int8_t out[HOME_LOC_COUNT]);
void home_layout_import(const int8_t in[HOME_LOC_COUNT]);
void home_widgets_export(home_widget_t out[HOME_WIDGETS]);
void home_widgets_import(const home_widget_t in[HOME_WIDGETS]);
void home_folders_export(int8_t members[OS_MAX_APPS][HOME_FOLDER_ITEMS], uint8_t counts[OS_MAX_APPS]);
void home_folders_import(const int8_t members[OS_MAX_APPS][HOME_FOLDER_ITEMS], const uint8_t counts[OS_MAX_APPS]);
bool home_app_installed(int id);
bool home_app_install(int id);
void home_unlock_anim(void);
void home_lock_veil(bool on);                    /* home screen stays empty behind the lock screen */
void home_set_hidden(bool hidden);

#endif /*OS_H*/
