#include "hardware.h"
#include "src/os/sdk.h"
#include <string.h>
#include <math.h>
static lv_obj_t * fixture, * pattern;
static lv_timer_t * timer;
static bool previous_blur, previous_lens;
static bool previous_auto, previous_full;
static void animate(lv_timer_t * t)
{
 (void)t;
 lv_obj_set_style_translate_x(pattern, (int)(30*sinf(lv_tick_get()*0.006f)), 0);
 lv_obj_invalidate(fixture);
}
void s3_effects(bool blur, bool lens)
{
 ui_effects_set(blur, lens); shell_settings_apply();
}
void s3_effect_test(const char * mode)
{
 if(fixture) {
  lv_timer_delete(timer); lv_obj_delete(fixture); fixture = pattern = NULL;
  s3_perf_report(); ui_effects_set(previous_blur, previous_lens);
  s3_render_mode(previous_full);
  if(previous_auto) s3_render_auto();
 }
 if(!strcmp(mode, "off")) return;
 previous_blur = ui_blur_enabled(); previous_lens = ui_lens_enabled();
 previous_auto=s3_render_is_auto();
 previous_full=lv_display_get_render_mode(lv_display_get_default())==LV_DISPLAY_RENDER_MODE_FULL;
 s3_render_mode(true);
 ui_effects_set(!strcmp(mode, "blur") || !strcmp(mode, "lens"), !strcmp(mode, "lens"));
 fixture = ui_box(lv_layer_top()); lv_obj_set_size(fixture, 384, 480);
 lv_obj_set_style_bg_color(fixture, lv_color_hex(0x101020), 0); lv_obj_set_style_bg_opa(fixture, 255, 0);
 pattern = ui_box(fixture); lv_obj_set_size(pattern, 444, 480); lv_obj_set_x(pattern, -30);
 for(int i = 0; i < 16; i++) {
  lv_obj_t * stripe = ui_box(pattern); lv_obj_set_pos(stripe, i*28, 0); lv_obj_set_size(stripe, 28, 480);
  lv_obj_set_style_bg_color(stripe, lv_color_hsv_to_rgb(i*23, 85, 95), 0); lv_obj_set_style_bg_opa(stripe, 255, 0);
 }
 lv_obj_t * glass = ui_box(fixture); lv_obj_set_size(glass, 240, 180); lv_obj_center(glass);
 lv_obj_set_style_radius(glass, 32, 0);
 if(!strcmp(mode, "lens")) ui_lens(glass, 14, 65, true); else ui_glass(glass, 0xffffff, 60, 18);
 ui_label(glass, mode, &lv_font_montserrat_24, UI_TEXT);
 timer = lv_timer_create(animate, 16, NULL); s3_perf_reset();
}
