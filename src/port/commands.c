#define S3_NATIVE_DRIVER 1
#include "hardware.h"
#include "board_config.h"
#include "s3_wifi.h"
#include "s3_doom.h"
#include "src/os/sdk.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern bool home_prepare_app(int id);
extern void cc_command_open(void);
extern bool s3_thorvg_self_test(void);
extern bool s3_vector_self_test(void);
extern void app_browser_navigate(const char * url);
static const char * TAG = "s3_cmd";
bool os_drop_poll(char * path, size_t n) { (void)path; (void)n; return false; }
bool s3_app_supported(int id)
{
 const char * name = os_apps[id]->name;
 return strcmp(name, "Music") &&
        strcmp(name, "Video") && strcmp(name, "Photos") && strcmp(name, "Weather");
}
static struct { lv_point_t from, to; uint32_t start, duration; bool active, started; } pointer;
/* Binary mouse bridge: A5, pressed(0/1), physical X, physical Y, XOR checksum. */
static struct { lv_point_t point; bool pressed; } remote[16];
static unsigned remote_head, remote_tail;
static bool remote_mode, remote_pressed;
static lv_point_t remote_point;
static uint32_t remote_last;
static unsigned remote_packets;
extern lv_image_dsc_t mouse_cursor_icon;
static lv_obj_t * mouse_cursor;
static void remote_packet(const uint8_t * p)
{
 if(p[0] > 1 || p[1] >= 128 || p[2] >= 160 || (uint8_t)(0xa5 ^ p[0] ^ p[1] ^ p[2]) != p[3]) return;
 remote_packets++; remote_mode = true; pointer.active = false; remote_last = lv_tick_get();
 if(mouse_cursor) lv_obj_remove_flag(mouse_cursor, LV_OBJ_FLAG_HIDDEN);
 unsigned last = (remote_head + 15) & 15;
 /* Coalesce motion, retain button edges until LVGL has sampled them. */
 if(remote_head != remote_tail && remote[last].pressed == (bool)p[0]) remote[last].point=(lv_point_t){p[1],p[2]};
 else {
  unsigned next=(remote_head+1)&15;
  if(next == remote_tail) remote_tail=(remote_tail+1)&15;
  remote[remote_head].point=(lv_point_t){p[1],p[2]}; remote[remote_head].pressed=p[0]; remote_head=next;
 }
}
static void pointer_read(lv_indev_t * in, lv_indev_data_t * data)
{
 (void)in;
 if(remote_mode) {
  if(remote_tail != remote_head) {
   remote_point=remote[remote_tail].point; remote_pressed=remote[remote_tail].pressed;
   remote_tail=(remote_tail+1)&15;
  }
  if(lv_tick_elaps(remote_last)>2000) remote_pressed=false;
  data->point=remote_point; data->state=remote_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  return;
 }
 if(pointer.active && !pointer.started) { pointer.start=lv_tick_get(); pointer.started=true; }
 uint32_t elapsed = lv_tick_elaps(pointer.start);
 float p = pointer.duration ? LV_MIN(1.0f, elapsed/(float)pointer.duration) : 1;
 data->point.x = pointer.from.x + (pointer.to.x-pointer.from.x)*p;
 data->point.y = pointer.from.y + (pointer.to.y-pointer.from.y)*p;
 data->state = pointer.active && elapsed < pointer.duration ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
 if(elapsed >= pointer.duration) pointer.active = false;
}
void s3_input_init(void)
{
 lv_indev_t * input = lv_indev_create(); lv_indev_set_type(input, LV_INDEV_TYPE_POINTER);
 lv_indev_set_read_cb(input, pointer_read);
 mouse_cursor = lv_image_create(lv_layer_sys());
 lv_image_set_src(mouse_cursor, &mouse_cursor_icon);
 lv_obj_remove_flag(mouse_cursor, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_add_flag(mouse_cursor, LV_OBJ_FLAG_HIDDEN);
 lv_indev_set_cursor(input, mouse_cursor);
 lv_timer_set_period(lv_indev_get_read_timer(input), 8);
}
static void gesture(int x, int y, int tx, int ty, int duration)
{
 if(x < 0 || x >= 128 || tx < 0 || tx >= 128 || y < 0 || y >= 160 || ty < 0 || ty >= 160 || duration < 40 || duration > 10000) return;
 remote_mode=false; remote_head=remote_tail=0;
 pointer.from = (lv_point_t){x,y}; pointer.to = (lv_point_t){tx,ty};
 pointer.duration = duration; pointer.start = lv_tick_get(); pointer.active = true; pointer.started = false;
}
static lv_obj_t * textarea(lv_obj_t * root)
{
 if(lv_obj_is_hidden(root)) return NULL;
 if(lv_obj_check_type(root, &lv_keyboard_class)) return lv_keyboard_get_textarea(root);
 if(lv_obj_check_type(root, &lv_textarea_class) && lv_obj_has_state(root, LV_STATE_FOCUSED)) return root;
 for(uint32_t i = 0; i < lv_obj_get_child_count(root); i++) {
  lv_obj_t * found = textarea(lv_obj_get_child(root, i)); if(found) return found;
 }
 return NULL;
}
void s3_command(const char * command)
{
 ESP_LOGI(TAG, "COMMAND %s", !strncmp(command,"wifi password ",14) ? "wifi password [redacted]" : command);
 if(s3_wifi_command(command)) return;
 int x,y,tx,ty,ms,a,b;
 char doom_key_name[16];
 if(!strncmp(command, "browser ", 8)) app_browser_navigate(command + 8);
 else if(sscanf(command, "doom key %15s %d", doom_key_name, &a) == 2 && (a == 0 || a == 1)) s3_doom_command(doom_key_name, a);
 else if(!strncmp(command, "app ", 4)) {
  char * end; long id = strtol(command + 4, &end, 10);
  if(*end || end == command + 4 || id < 0 || id >= os_app_count || !shell_home_idle() || lock_visible()) {
   ESP_LOGW(TAG, "App requires a valid ID and settled Home screen"); return;
  }
  lv_area_t area;
  if(home_prepare_app(id) && home_get_icon_area(id, &area)) shell_launch_from_home(id, &area);
  else shell_open_app(id);
 }
 else if(sscanf(command, "tap %d %d", &x, &y) == 2) gesture(x,y,x,y,80);
 else if(sscanf(command, "hold %d %d %d", &x, &y, &ms) == 3) gesture(x,y,x,y,ms);
 else if(sscanf(command, "drag %d %d %d %d %d", &x,&y,&tx,&ty,&ms) == 5) gesture(x,y,tx,ty,ms);
 else if(!strncmp(command, "type ", 5)) {
  lv_obj_t * field = textarea(lv_screen_active()); if(!field) field = textarea(lv_layer_top());
  if(field) lv_textarea_add_text(field, command+5);
 }
 else if(!strcmp(command, "back")) s3_back();
 else if(!strcmp(command, "status")) ESP_LOGI(TAG,"STATUS app=%d busy=%d recents=%d cards=%d cc=%d shade=%d notifications=%d pointer_packets=%u x=%d y=%d pressed=%d",s3_app_active(),s3_shell_busy(),s3_recents_visible(),s3_recents_count(),cc_visible(),lock_visible(),notify_count(),remote_packets,remote_point.x,remote_point.y,remote_pressed);
 else if(!strcmp(command, "recents pull")) gesture(64,158,64,65,650);
 else if(!strcmp(command, "recents clear")) s3_recents_clear_all();
 else if(!strcmp(command, "test vector")) ESP_LOGI(TAG, "%s ThorVG vector renderer/SIMD integration", s3_vector_self_test() ? "PASS" : "FAIL");
 else if(!strcmp(command, "test simd")) {
  ESP_LOGI(TAG, "%s SIMD fill/copy/guards/fallback", s3_simd_self_test() ? "PASS" : "FAIL");
  ESP_LOGI(TAG, "%s ThorVG ARGB SIMD alignment/tails/guards", s3_thorvg_self_test() ? "PASS" : "FAIL");
 }
 else if(!strcmp(command, "recents clear-one")) s3_recents_clear_one();
 else if(!strcmp(command, "perf")) s3_perf_report();
 else if(!strcmp(command, "screen")) s3_screenshot();
 else if(!strcmp(command, "profile on")) s3_profile(true);
 else if(!strcmp(command, "profile off")) s3_profile(false);
 else if(!strcmp(command, "perf reset")) s3_perf_reset();
 else if(!strcmp(command, "perf on")) s3_perf_logging(true);
 else if(!strcmp(command, "perf off")) s3_perf_logging(false);
 else if(!strcmp(command, "render full")) s3_render_mode(true);
 else if(!strcmp(command, "render direct")) s3_render_mode(false);
 else if(!strcmp(command, "render auto")) s3_render_auto();
 else if(!strcmp(command, "render tiled")) s3_render_tiles();
 else if(!strcmp(command, "filter box")) s3_filter(true);
 else if(!strcmp(command, "filter fast")) s3_filter(false);
 else if(!strncmp(command, "effects test ", 13)) s3_effect_test(command+13);
 else if(!strcmp(command, "effects off")) s3_effects(false,false);
 else if(!strcmp(command, "effects blur")) s3_effects(true,false);
 else if(!strcmp(command, "effects lens") || !strcmp(command, "effects both")) s3_effects(true,true);
 else if(!strcmp(command, "lock")) lock_now();
 else if(!strcmp(command, "edit")) home_edit_enter();
 else if(!strcmp(command, "done")) home_edit_exit();
 else if(sscanf(command, "widget add %d", &a) == 1 && a >= 0 && a < os_widget_count) home_widget_add(a);
 else if(sscanf(command, "widget remove %d", &a) == 1 && a >= 0 && a < os_widget_count) home_widget_remove(a);
 else if(!strcmp(command, "widgets")) { for(int i=0;i<os_widget_count;i++) ESP_LOGI(TAG,"%d %s",i,os_widgets[i]->name); }
 else if(sscanf(command, "folder add %d %d", &a, &b) == 2 && a >= 0 && a < os_app_count && b >= 0 && b < os_app_count && a != b) {
  char c[40]; snprintf(c,sizeof(c),"fold=%d:%d",a,b); home_debug_cmd(c);
 }
 else if(sscanf(command, "folder open %d", &a) == 1 && a >= 0 && a < os_app_count) { char c[32]; snprintf(c,sizeof(c),"fopen=%d",a); home_debug_cmd(c); }
 else if(!strcmp(command, "folder close")) home_debug_cmd("fclose");
 else if(!strcmp(command, "save")) os_save();
 else if(!strcmp(command, "home")) s3_app_close();
 else if(!strcmp(command, "recents")) shell_recents_open();
 else if(!strcmp(command, "unlock")) lock_debug_unlock();
 else if(!strcmp(command, "cc open")) cc_command_open();
 else if(!strcmp(command, "cc close")) cc_close();
 else if(!strcmp(command, "shade open")) s3_notify_open(true);
 else if(!strcmp(command, "shade close")) s3_notify_open(false);
 else if(!strcmp(command, "notify")) notify_post(0, "Test notification", "Original notification animation");
 else if(!strcmp(command, "clear")) notify_clear();
 else if(!strcmp(command, "clear-one")) s3_notify_clear_one();
 else if(!strcmp(command, "test")) s3_smoke_start();
 else if(!strcmp(command, "stop")) s3_smoke_cancel();
 else if(!strcmp(command, "apps")) {
  for(int i = 0; i < os_app_count; i++) ESP_LOGI(TAG, "%d %s", i, os_apps[i]->name);
 }
 else ESP_LOGI(TAG, "Commands: status, apps, app ID, home, back, recents [pull|clear-one|clear], lock, unlock, cc/shade open/close, notify, clear[-one], tap X Y, hold X Y MS, drag X Y X2 Y2 MS, type TEXT, edit, done, widgets, widget add/remove ID, folder add A B/open ID/close, save, effects off/blur/lens/both/test plain|blur|lens|off, render auto/direct/full, perf [on|off|reset], profile on/off, screen, test [simd|vector], stop, browser URL, wifi status/scan/ssid NAME/password PASSWORD/connect/disconnect, doom key up/down/left/right/fire/use/enter/escape 0|1. Coordinates: physical 128x160.");
}
void s3_command_poll(void)
{
 static bool initialized, overflow;
 static char line[96]; static unsigned length;
 static uint8_t binary[4]; static unsigned binary_length; static bool in_binary;
 if(!initialized) {
  usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg)); initialized = true;
 }
 char input[256]; int n = usb_serial_jtag_read_bytes(input, sizeof(input), 0);
 for(int i = 0; i < n; i++) {
  char ch = input[i];
  if(in_binary) {
   binary[binary_length++]=(uint8_t)ch;
   if(binary_length==4) { remote_packet(binary); in_binary=false; binary_length=0; }
   continue;
  }
  if((uint8_t)ch==0xa5 && length == 0) { in_binary=true; binary_length=0; continue; }
  if(ch == '\r' || ch == '\n') {
   if(length && !overflow) { line[length] = 0; s3_command(line); }
   length = 0; overflow = false;
  } else if(ch == 8 || ch == 127) { if(length) length--; }
  else if((uint8_t)ch >= 32) {
   if(length < sizeof(line)-1) line[length++] = ch; else overflow = true;
  }
 }
}
