/**
 * @file main.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#ifndef _DEFAULT_SOURCE
  #define _DEFAULT_SOURCE /* needed for usleep() */
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#ifdef _MSC_VER
  #include <Windows.h>
#else
  #include <unistd.h>
  #include <pthread.h>
#endif
#include "lvgl/lvgl.h"
#ifdef S3_RTOS
#include "FreeRTOS.h"
#include "task.h"
#endif
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include <SDL.h>

#include "hal/hal.h"
#include "os/os.h"


#if LV_USE_OS != LV_OS_FREERTOS

static void dbg_exit_cb(lv_timer_t * t)
{
  (void)t;
  home_edit_exit();
}

static void dbg_drop_cb(lv_timer_t * t)
{
  (void)t;
  home_debug_wdrop();
}

static void dbg_seq_cb(lv_timer_t * t)
{
  /* ESPRING_SEQ="fold=1:2|fopen=1|..." : one step every 1.2 s; "home" closes the app */
  static char buf[512];
  static char * cur;
  if(!cur) { strncpy(buf, lv_timer_get_user_data(t), sizeof(buf) - 1); cur = buf; }
  if(!*cur) { lv_timer_delete(t); return; }
  char * end = strchr(cur, '|');
  if(end) *end = 0;
  if(strcmp(cur, "home") == 0) shell_debug_home();
  else if(strncmp(cur, "app=", 4) == 0) shell_open_app(atoi(cur + 4));
  else if(strncmp(cur, "lstyle=", 7) == 0) { os_settings.lock_style = (uint8_t)atoi(cur + 7); lock_refresh(); }
  else if(strcmp(cur, "unlock") == 0) lock_debug_unlock();
  else home_debug_cmd(cur);
  cur = end ? end + 1 : cur + strlen(cur);
}

/* Simulator test hook: ESPRING_DBG="cc=0.5" / "unlock" / "shade=0.8" runs once, shortly after boot */
static void dbg_cb(lv_timer_t * t)
{
  const char * d = lv_timer_get_user_data(t);
  float v = 0;
  if(strstr(d, "notifs=")) {
    char b[48];
    for(int i = 0; i < atoi(strstr(d, "notifs=") + 7); i++) {
      snprintf(b, sizeof(b), "Message %d from a friend", i + 1);
      notify_post(i % os_app_count, b, "Hey! This is a long enough notification body to wrap onto a second line.");
    }
  }
  if(strstr(d, "unlock")) lock_debug_unlock();
  if(strstr(d, "pop")) cc_debug_pop();
  if(strstr(d, "cc=")) { v = (float)atof(strstr(d, "cc=") + 3); cc_debug_pose(v); }
  if(strstr(d, "shade=")) { v = (float)atof(strstr(d, "shade=") + 6); lock_debug_shade(v); }
  if(strstr(d, "drop=")) import_offer(strstr(d, "drop=") + 5);
  if(strstr(d, "app=")) shell_open_app(atoi(strstr(d, "app=") + 4));
  if(strstr(d, "rsz=")) home_debug_resize(atoi(strstr(d, "rsz=") + 4), atoi(strstr(d, ":") + 1));
  if(strstr(d, "wres=")) {
    int k, ax, dx, dy;
    if(sscanf(strstr(d, "wres=") + 5, "%d:%d:%d:%d", &k, &ax, &dx, &dy) == 4) home_debug_wres(k, ax, dx, dy);
  }
  if(strstr(d, "wdrag=")) {
    int k, x, y;
    if(sscanf(strstr(d, "wdrag=") + 6, "%d:%d:%d", &k, &x, &y) == 3) home_debug_wdrag(k, x, y);
  }
  if(strstr(d, "wdrop")) lv_timer_set_repeat_count(lv_timer_create(dbg_drop_cb, 1600, NULL), 1);
  if(strstr(d, "edit")) home_edit_enter();
  if(strstr(d, "exitedit")) lv_timer_set_repeat_count(lv_timer_create(dbg_exit_cb, 1800, NULL), 1);
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
static void web_frame(void)
{
  lv_timer_handler();
}
#endif

static void os_run(void)
{

  /*Initialize LVGL*/
  lv_init();

  /*Initialize the HAL (display, input devices, tick) for LVGL*/
  lv_display_t * disp = sdl_hal_init(320, 480);
  lv_sdl_window_set_title(disp, "S3-Springboard");

  shell_init();
  if(getenv("ESPRING_SEQ")) lv_timer_create(dbg_seq_cb, 1200, getenv("ESPRING_SEQ"));
  if(getenv("ESPRING_DBG")) lv_timer_set_repeat_count(lv_timer_create(dbg_cb, 900, getenv("ESPRING_DBG")), 1);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(web_frame, 0, 1);
  return;
#endif
  while(1) {
    /* Periodically call the lv_task handler.
     * It could be done in a timer interrupt or an OS task too.*/
    uint32_t sleep_time_ms = lv_timer_handler();
    if(sleep_time_ms == LV_NO_TIMER_READY){
	sleep_time_ms =  LV_DEF_REFR_PERIOD;
    }
#if defined(S3_RTOS)
    vTaskDelay(pdMS_TO_TICKS(sleep_time_ms));
#elif defined(_MSC_VER)
    Sleep(sleep_time_ms);
#else
    usleep(sleep_time_ms * 1000);
#endif
  }

}

#ifdef S3_RTOS
static void os_task(void * arg)
{
  (void)arg;
  os_run();
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  /* The whole shell runs inside a FreeRTOS task (Windows MinGW simulator port) */
  xTaskCreate(os_task, "S3-Springboard", 16384, NULL, 2, NULL);
  vTaskStartScheduler();
  return 0;
}
#else
int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  os_run();
  return 0;
}
#endif


#endif
