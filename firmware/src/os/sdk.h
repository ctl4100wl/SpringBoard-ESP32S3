/**
 * @file sdk.h
 * App SDK: everything an app needs besides LVGL itself.
 *
 *   #include "../os/sdk.h"
 *
 *   static void tick(lv_timer_t * t) { ... }
 *
 *   APP(app_hello, "Hello", LV_SYMBOL_HOME, 0x0A84FF)
 *   {
 *       lv_obj_t * body = ui_app_scaffold(root, "Hello");     // title + scrolling body
 *       int n = sdk_get_int("hello.count", 0);                 // persisted key/value
 *       sdk_set_int("hello.count", n + 1);
 *       sdk_timer(root, tick, 1000, NULL);                     // auto-deleted with the app
 *       sdk_http_get("https://example.com", on_reply, NULL, root); // async, cb skipped if app closed
 *   }
 *
 * Then add `&app_hello` to src/apps/apps.c. See src/apps/README.md.
 */
#ifndef SDK_H
#define SDK_H

#include "os.h"
#include "ui.h"

/** Define an app: expands to its descriptor + the create function header (`root` is the app window) */
#define APP(var, name, symbol, color)                                       \
    static void var##_create(lv_obj_t * root);                              \
    const app_desc_t var = { name, symbol, color, var##_create };           \
    static void var##_create(lv_obj_t * root)

/** Widget sizes in home-grid cells */
typedef enum { WIDGET_SMALL, WIDGET_MEDIUM, WIDGET_LARGE } widget_size_t;   /* 2x2, 4x2, 4x4 */

/** Home screen widget. Linked to `app`: tapping opens it, closing the app morphs back into the widget.
 *  `create` fills the box (use sdk_timer(box, ...) to refresh). `lock_create` (optional) fills a
 *  small inline slot on the lock screen. Users add/remove widgets in jiggle mode ("+" button). */
typedef struct {
    const char * name;
    const app_desc_t * app;
    widget_size_t size;
    void (*create)(lv_obj_t * box);
    void (*lock_create)(lv_obj_t * slot);
    bool resizable;         /* adapts its layout to every size; the user can resize it in jiggle mode */
} widget_desc_t;

/** Define a widget: WIDGET(widget_foo, "Foo", &app_foo, WIDGET_SMALL, foo_lock_fn_or_NULL) { ...uses box... } */
#define WIDGET(var, name, app, size, lock_fn)                                     \
    static void var##_create(lv_obj_t * box);                                     \
    const widget_desc_t var = { name, app, size, var##_create, lock_fn, false };  \
    static void var##_create(lv_obj_t * box)

/** Like WIDGET but the widget lays itself out for any size: read sdk_widget_size() in the create function
 *  (it is called again after the user resizes the widget). `size` is the default. */
#define WIDGET_RESIZABLE(var, name, app, size, lock_fn)                           \
    static void var##_create(lv_obj_t * box);                                     \
    const widget_desc_t var = { name, app, size, var##_create, lock_fn, true };   \
    static void var##_create(lv_obj_t * box)

/** Size the widget is currently being built for (valid inside its create function) */
widget_size_t sdk_widget_size(void);
void sdk_widget_size_set(widget_size_t size);       /* used by the home screen */
int sdk_widget_inst(void);                          /* which placed instance (0..5) is being built: per-widget settings */
void sdk_widget_inst_set(int k);

extern const widget_desc_t * const os_widgets[];
extern const int os_widget_count;

/*---- Lifetime-bound timers ----*/
lv_timer_t * sdk_timer(lv_obj_t * owner, lv_timer_cb_t cb, uint32_t period_ms, void * user_data);

/*---- Persistent key/value storage (apps.kv) ----*/
int sdk_get_int(const char * key, int def);
/** Create a folder (and its parents) relative to the program; used for download caches such as "cache/photos". */
void sdk_ensure_dir(const char * path);
void sdk_set_int(const char * key, int value);
const char * sdk_get_str(const char * key, const char * def);
void sdk_set_str(const char * key, const char * value);

/*---- Networking (background thread, result delivered on the UI thread) ----*/
typedef void (*sdk_http_cb_t)(bool ok, const char * body, void * user_data);
/** `owner` may be NULL; if given and deleted before the reply arrives, `cb` is not called. */
void sdk_http_get(const char * url, sdk_http_cb_t cb, void * user_data, lv_obj_t * owner);

/** Binary reply (up to 4 MB), e.g. an image. The buffer is only valid inside the callback. */
typedef void (*sdk_http_data_cb_t)(bool ok, const uint8_t * data, size_t len, void * user_data);
void sdk_http_get_data(const char * url, sdk_http_data_cb_t cb, void * user_data, lv_obj_t * owner);
/** Same with a custom size limit (default 4 MB), for big downloads. */
void sdk_http_get_data_ex(const char * url, sdk_http_data_cb_t cb, void * user_data, lv_obj_t * owner, size_t max_bytes);

/** Blocking fetch for use INSIDE a background job (never call it on the UI thread). Returns bytes read (0 = failed). */
size_t sdk_http_fetch_sync(const char * url, char * buf, size_t cap);

/** Run `work` on a background thread; `done(user_data, ok)` runs on the UI thread afterwards (ok = false if the job
 *  overran `timeout_ms` and was abandoned). Skipped if `owner` was deleted meanwhile. */
typedef void (*sdk_job_fn_t)(void * user_data);
typedef void (*sdk_job_done_t)(void * user_data, bool ok);
void sdk_job_run(sdk_job_fn_t work, sdk_job_done_t done, void * user_data, lv_obj_t * owner, uint32_t timeout_ms);
bool sdk_online(void);  /* result of the most recent request */

/*---- Tiny JSON helpers (flat lookups, good enough for REST replies) ----*/
const char * sdk_json_find(const char * json, const char * key);  /* points just after "key": */
bool sdk_json_num(const char * json, const char * key, double * out);
int sdk_json_num_array(const char * json, const char * key, double * out, int max);

/*---- Notifications: iOS-style banner when unlocked, stacked on the lock screen otherwise ----*/
void sdk_notify(const app_desc_t * app, const char * title, const char * body);

/*---- Pull to refresh on a scrolling body (e.g. from ui_app_scaffold) ----*/
typedef void (*sdk_refresh_cb_t)(lv_obj_t * body, void * user_data);
void sdk_pull_refresh(lv_obj_t * body, sdk_refresh_cb_t cb, void * user_data);
void sdk_pull_refresh_done(lv_obj_t * body);    /* hide the spinner when the data is in */

/*---- Images from the PC ----*/
bool sdk_pick_file(const char * title, const char * filter, char * path_out, size_t n);   /* filter: "Name\0*.ext;*.ext\0" */
bool sdk_pick_image(char * path_out, size_t n);
/** File picker that also works in the browser build (there the dialog is asynchronous). `path` is NULL if cancelled. */
typedef void (*sdk_pick_cb_t)(const char * path, void * user_data);
void sdk_pick_file_async(const char * title, const char * filter, sdk_pick_cb_t cb, void * user_data);         /* native file dialog; false if cancelled/unsupported */
lv_draw_buf_t * sdk_image_load(const char * path, int32_t w, int32_t h);   /* decoded + cover-cropped, RGB565 */
/** Decode PNG / JPEG bytes in memory, scaled down to fit max_w x max_h keeping the aspect ratio (RGB565). NULL if unsupported. */
lv_draw_buf_t * sdk_image_decode_mem(const uint8_t * data, size_t len, int32_t max_w, int32_t max_h);

/*---- System UI ----*/
void sdk_toast(const char * title, const char * text);
void sdk_open_app(const app_desc_t * app);
int sdk_app_index(const app_desc_t * app);

#endif /*SDK_H*/
