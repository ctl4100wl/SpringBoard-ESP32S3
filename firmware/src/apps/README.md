# ESPringBoard app SDK

An app is one C file in `src/apps/` plus one line in `apps.c`. Include `../os/sdk.h` and you get LVGL,
the UI helpers (`ui.h`), spring physics (`fx.h`) and the system services below.

## Hello app

```c
#include "../os/sdk.h"

static void on_reply(bool ok, const char * body, void * ud) { /* runs on the UI thread */ }

APP(app_hello, "Hello", LV_SYMBOL_HOME, 0x0A84FF)          /* `root` = the app window */
{
    lv_obj_t * body = ui_app_scaffold(root, "Hello");          /* large title + scrolling body */
    lv_obj_t * card = ui_section(body, "OPTIONS");
    ui_switch_row(card, LV_SYMBOL_BELL, UI_RED, "Enabled", sdk_get_int("hello.on", 1), NULL, NULL);
    ui_nav_row(card, NULL, 0, "Details", "3", details_cb, NULL);   /* push a page with ui_nav_push() */
    sdk_timer(root, tick, 1000, NULL);                         /* deleted with the window */
    sdk_http_get("https://example.com/api", on_reply, NULL, root);  /* skipped if the app closed */
}
```

Register it in `apps.c` (`extern const app_desc_t app_hello;` and `&app_hello` in `os_apps[]`).
The window is destroyed when the user leaves; keep state in statics or `sdk_set_*`.

## Widgets (linked to an app)

```c
static void lock_slot(lv_obj_t * slot) { ui_label(slot, "42 steps", &lv_font_montserrat_14, 0xFFFFFF); }

WIDGET(widget_hello, "Hello", &app_hello, WIDGET_SMALL, lock_slot)   /* SMALL 2x2, MEDIUM 4x2, LARGE 4x4 */
{
    ui_label(box, "Hello", &lv_font_montserrat_20, 0xFFFFFF);
    sdk_timer(box, refresh, 5000, box);                        /* refresh while the widget exists */
}
```

Add `&widget_hello` to `os_widgets[]` in `apps.c`. Users add it from jiggle mode (long-press home, "+").
Tapping it opens `app_hello`; leaving the app morphs the window back into the widget.
The optional lock function fills a small slot under the Lock Screen clock (pass `NULL` to skip).

### Moving and resizing widgets

In jiggle mode every widget can be dragged to another cell or page (a ghost outline shows where it will land).
Resizable widgets also get two curved handles on the right and bottom edge: the frame follows your finger, then
springs to the nearest grid size (Small 2x2, Medium 4x2, Large 4x4). Content sits in a fixed-size `inner` pinned to
the top left of the frame, so text never reflows mid-drag; the new layout fades in once the size is chosen.
`sdk_widget_inst()` tells a widget which placed instance it is (0..5) for per-widget settings.

### Resizable widgets

`WIDGET_RESIZABLE(var, name, &app, WIDGET_SMALL, lock_fn)` declares a widget that lays itself out for every size:
read `sdk_widget_size()` (SMALL / MEDIUM / LARGE) in the body and draw for that size. In jiggle mode, tapping the
widget shows S / M / L chips; the frame springs to the new size and the body is built again for it
(see `widget_rings` and `widget_daylight` in `widgets_extra.c`). Widgets can also be added / removed in
Settings > Home Screen > Widgets (`home_widget_add/remove/count` in `os.h`).

## Background work

`sdk_job_run(work, done, ud, owner, timeout_ms)` runs `work` on a thread and calls `done` on the UI thread.
`sdk_http_get_data` returns binary replies (images), `sdk_image_decode_mem` decodes PNG / JPEG bytes into an RGB565
buffer, and `sdk_http_fetch_sync` is the blocking fetch to use inside a job. The Lite Browser uses all of them plus
QuickJS (`src/third_party/quickjs`, MIT licence) for JavaScript. Its `about:js` page tests modern syntax and DOM output.
The browser remains a text-first HTML view with a small DOM shim, not a full CSS layout engine.

## Services

| Area | API |
| --- | --- |
| Timers | `sdk_timer(owner, cb, ms, ud)` - auto-deleted with `owner` |
| Storage | `sdk_get_int/str`, `sdk_set_int/str` (persisted in `apps.kv`) |
| Network | `sdk_http_get(url, cb, ud, owner)`, `sdk_online()`; JSON: `sdk_json_find/num/num_array` |
| Notifications | `sdk_notify(&app, title, body)` - banner when unlocked, Lock Screen stack otherwise; `sdk_toast(title, body)` |
| Pull to refresh | `sdk_pull_refresh(scroll_obj, cb, ud)`, then `sdk_pull_refresh_done(scroll_obj)` when data arrives |
| Images | `sdk_pick_image(path, n)` (native file dialog), `sdk_image_load(path, w, h)` (decoded, cover-cropped buffer) |
| Navigation | `ui_nav_push(root, title, back)` returns the new page body; back pops with a spring |
| Physics | `fx_to(obj, FX_Y / FX_X / FX_SCALE / FX_OPA, target, response, damping, delay, done)`, `fx_set`, `fx_kick` |
| System | `sdk_open_app(&app)`, `sdk_app_index(&app)`, `lock_now()`, `os_settings` |

`fx_*` springs are interruptible (retargeting keeps velocity) and use one timer with a fixed pool, so
they are cheap enough for the ESP32-S3. Presets: `FX_SNAPPY`, `FX_BOUNCY`, `FX_SOFT`.

## Liquid glass (refraction)

`ui_lens(obj, bezel, strength, chroma)` turns any rounded object into a frosted pane whose rim bends the content
behind it (convex-squircle lens profile + Snell's law, rim specular, optional red/blue fringe).
A displacement table (3 bytes per rim pixel) is built once per pane size; each frame the rim is rewritten from a
sharp copy of the backdrop. `src/os/lens.c` explains the maths. It needs two small LVGL patches
(`patches/lvgl-blur-hook.patch`: a hook around the software blur task, and
`patches/lvgl-obj-blur-radius.patch`: backdrop blur ignored the corner radius). Apply each from the repository
root with `patch -p1 -d lvgl < patches/<file>`. Gated by `OS_FX_LENS` (defaults to `OS_FX_BLUR`).

Try it: SDK Test app -> "Liquid glass lab" (drag the glass, change bezel / thickness / fringe).
The Glass Lock Screen clock uses the same saved bezel, thickness and fringe settings.

## Image import (simulator)

Drop a PNG / JPG / BMP on the window: a sheet asks for Home, Lock or Both and sets it as wallpaper
(`import_offer(path)` in `os/import.c`; `hal.c` delivers the dropped path).

## SDK Test app and widgets

`app_sdktest.c` exercises every service above (storage, timers, notifications, network, pull to refresh, images,
springs, navigation) and is the best cookbook. `widgets_extra.c` holds the battery, calendar, now playing, notes and
activity widgets.

## ESP32-S3 notes

* `OS_FX_BLUR` (backdrop blur) and `OS_FX_HEAVY` (scale transforms) default to 0 when `ESP_PLATFORM`
  is defined; glass surfaces fall back to a denser tint. Override in your build flags.
* `sdk_http_get` uses `curl` + pthreads on the PC; on the ESP32 replace `http_worker` in `sdk.c` with
  `esp_http_client`. `sdk_pick_image` returns false off Windows; load images from SPIFFS/SD with
  `sdk_image_load("/sd/x.png", w, h)` (the LVGL `A:` stdio drive).
* The simulator uses the C library malloc (`lv_conf.h`); on the device use PSRAM for LVGL's heap.
