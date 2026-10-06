/**
 * @file fx.h
 * Tiny spring-physics animator for object properties.
 *
 * One shared timer steps a fixed pool of damped springs (no malloc, ESP32-S3 friendly).
 * Springs are interruptible: retargeting keeps the current velocity, so motion stays continuous
 * when a finger grabs something mid-flight.
 *
 *   fx_set(obj, FX_Y, -40);                              // jump
 *   fx_to(obj, FX_Y, 0, 0.45f, 0.72f, 60, NULL);         // spring to 0 after 60 ms (bouncy)
 *   fx_kick(obj, FX_Y, 900);                             // add release velocity (units/s)
 */
#ifndef FX_H
#define FX_H

#include "lvgl/lvgl.h"

/* Heavy effects (backdrop blur, full-screen transforms). Off on the ESP32 by default. */
#ifndef OS_FX_BLUR
  #ifdef ESP_PLATFORM
    #define OS_FX_BLUR  0
  #else
    #define OS_FX_BLUR  1
  #endif
#endif
#ifndef OS_FX_HEAVY
  #define OS_FX_HEAVY OS_FX_BLUR
#endif

/* Refractive "liquid glass" rim (ui_lens, lens.c). Needs patches/lvgl-blur-hook.patch; off on the ESP32 until profiled */
#ifndef OS_FX_LENS
  #define OS_FX_LENS  OS_FX_BLUR
#endif

typedef enum {
    FX_X,       /* translate_x, px */
    FX_Y,       /* translate_y, px */
    FX_SCALE,   /* transform scale, 256 = 1.0 (pivot = center) */
    FX_OPA,     /* opa 0..255 */
    FX_DW,       /* extra width in px (transform_width): squish / stretch */
    FX_DH,       /* extra height in px */
    FX_PROP_COUNT
} fx_prop_t;

typedef void (*fx_done_cb_t)(lv_obj_t * obj);

/* Response = period in seconds (smaller = snappier), damping 1 = no overshoot, < 1 = bouncy */
#define FX_SNAPPY   0.30f, 0.86f
#define FX_BOUNCY   0.45f, 0.68f
#define FX_SOFT     0.55f, 0.90f

void fx_set(lv_obj_t * obj, fx_prop_t prop, float value);
void fx_to(lv_obj_t * obj, fx_prop_t prop, float target, float response, float damping, uint32_t delay_ms,
           fx_done_cb_t done);
void fx_kick(lv_obj_t * obj, fx_prop_t prop, float velocity);
float fx_get(lv_obj_t * obj, fx_prop_t prop);
bool fx_busy(lv_obj_t * obj, fx_prop_t prop);
bool fx_any_busy(void);
void fx_stop(lv_obj_t * obj);

#include "native_layout.h"
#endif /*FX_H*/
