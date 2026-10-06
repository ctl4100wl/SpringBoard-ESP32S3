#pragma once
#include <stdbool.h>
#include "lvgl/lvgl.h"
void s3_wifi_poll(void);
void s3_wifi_restore(void);
bool s3_wifi_command(const char * command);
void s3_wifi_settings(lv_obj_t * body);
