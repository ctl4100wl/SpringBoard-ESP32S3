#ifndef S3_HARDWARE_H
#define S3_HARDWARE_H
#include <stdbool.h>
#include "lvgl/lvgl.h"
void springboard_hardware_start(void);
void s3_smoke_start(void);
void s3_smoke_cancel(void);
bool s3_smoke_running(void);
void s3_command(const char * command);
void s3_command_poll(void);
void s3_input_init(void);
bool s3_app_supported(int id);
bool s3_shell_busy(void);
bool s3_app_active(void);
void s3_app_close(void);
bool s3_notify_busy(void);
bool s3_cc_busy(void);
bool s3_shade_busy(void);
void s3_notify_open(bool open);
void s3_notify_clear_one(void);
unsigned s3_flush_count(void);
void s3_perf_report(void);
void s3_perf_reset(void);
void s3_perf_logging(bool enabled);
void s3_filter(bool filtered);
void s3_render_mode(bool full);
void s3_render_auto(void);
bool s3_render_is_auto(void);
void s3_render_tiles(void);
bool s3_render_is_tiled(void);
void s3_effects(bool blur, bool lens);
void s3_effect_test(const char * mode);
int s3_recents_count(void);
bool s3_recents_visible(void);
void s3_recents_clear_one(void);
void s3_recents_clear_all(void);
bool s3_simd_self_test(void);
void s3_back(void);
void s3_profile(bool on);
void s3_screenshot(void);
#endif
