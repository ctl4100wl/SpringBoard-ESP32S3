#define S3_NATIVE_DRIVER 1
#include "s3_wifi.h"
#include "src/os/os.h"
#include "src/os/sdk.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static bool initialized, running;
static bool sntp_started;
static _Atomic bool connected, scanning;
static uint32_t retry_at;
static wifi_config_t config;
static void event(void * arg, esp_event_base_t base, int32_t id, void * data)
{
    (void)arg;
    if(base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        connected = true;
        ESP_LOGI("wifi_ui", "Connected; IP=" IPSTR, IP2STR(&((ip_event_got_ip_t *)data)->ip_info.ip));
        if(!sntp_started) {
            esp_sntp_config_t time_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
            sntp_started = esp_netif_sntp_init(&time_config) == ESP_OK;
        }
    }
    else if(base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        connected = false;
        ESP_LOGI("wifi_ui", "Disconnected (reason %u)", ((wifi_event_sta_disconnected_t *)data)->reason);
    }
    else if(base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
        uint16_t count = 0;
        esp_wifi_scan_get_ap_num(&count);
        esp_wifi_clear_ap_list(); scanning = false;
        ESP_LOGI("wifi_ui", "Scan complete: %u networks", count);
    }
}
static bool init(void)
{
    if(initialized) return true;
    esp_err_t result = esp_event_loop_create_default();
    if(result != ESP_OK && result != ESP_ERR_INVALID_STATE) return false;
    if(!esp_netif_create_default_wifi_sta()) return false;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.static_rx_buf_num = 4; cfg.dynamic_rx_buf_num = 8;
    cfg.static_tx_buf_num = 4; cfg.dynamic_tx_buf_num = 8; cfg.cache_tx_buf_num = 8;
    if(esp_wifi_init(&cfg) != ESP_OK) return false;
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event, NULL));
    nvs_handle_t n;
    if(nvs_open("wifi_ui", NVS_READONLY, &n) == ESP_OK) {
        size_t size = sizeof(config); nvs_get_blob(n, "station", &config, &size); nvs_close(n);
    }
    initialized = true;
    return true;
}
void s3_wifi_poll(void)
{
    if(!initialized) return;
    bool enabled = os_settings.wifi && !os_settings.airplane;
    if(enabled && !running && config.sta.ssid[0]) {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
        if(esp_wifi_start() == ESP_OK) {
            running = true;
            esp_wifi_set_ps(WIFI_PS_NONE);
            esp_wifi_connect();
            retry_at = lv_tick_get() + 10000;
        }
    }
    else if(!enabled && running) { esp_wifi_stop(); running = false; connected = false; }
    else if(enabled && running && !connected && !scanning && config.sta.ssid[0] && (int32_t)(lv_tick_get() - retry_at) >= 0) {
        retry_at = lv_tick_get() + 10000; esp_wifi_connect();
    }
}
void s3_wifi_restore(void)
{
    nvs_handle_t n;
    if(nvs_open("wifi_ui", NVS_READONLY, &n) != ESP_OK) return;
    size_t size = sizeof(config);
    bool saved = nvs_get_blob(n, "station", &config, &size) == ESP_OK;
    nvs_close(n);
    if(saved && config.sta.ssid[0]) { init(); s3_wifi_poll(); }
}
bool s3_wifi_command(const char * c)
{
    if(strncmp(c, "wifi ", 5)) return false;
    if(!init()) { ESP_LOGE("wifi_ui", "Wi-Fi initialization failed"); return true; }
    if(!strcmp(c, "wifi scan")) {
        os_settings.wifi = true; os_settings.airplane = false;
        if(!running && esp_wifi_start() == ESP_OK) running = true;
        scanning = true;
        if(esp_wifi_scan_start(NULL, false) != ESP_OK) { scanning = false; ESP_LOGE("wifi_ui", "Scan failed"); }
    }
    else if(!strncmp(c, "wifi ssid ", 10) && strlen(c + 10) <= 32) {
        memset(config.sta.ssid, 0, sizeof(config.sta.ssid)); memcpy(config.sta.ssid, c + 10, strlen(c + 10));
    }
    else if(!strncmp(c, "wifi password ", 14) && strlen(c + 14) <= 63) {
        memset(config.sta.password, 0, sizeof(config.sta.password)); memcpy(config.sta.password, c + 14, strlen(c + 14));
    }
    else if(!strcmp(c, "wifi connect")) {
        if(running) { esp_wifi_stop(); running = false; }
        nvs_handle_t n;
        if(nvs_open("wifi_ui", NVS_READWRITE, &n) == ESP_OK) { nvs_set_blob(n, "station", &config, sizeof(config)); nvs_commit(n); nvs_close(n); }
        os_settings.wifi = true; os_settings.airplane = false; s3_wifi_poll();
    }
    else if(!strcmp(c, "wifi disconnect")) { os_settings.wifi = false; s3_wifi_poll(); }
    else ESP_LOGI("wifi_ui", "Wi-Fi %s; configure with wifi ssid NAME, wifi password PASSWORD, wifi connect", connected ? "connected" : running ? "connecting/disconnected" : "off");
    return true;
}

typedef struct { lv_obj_t * ssid, * password, * keyboard, * status; } wifi_ui_t;
static void ui_status(lv_timer_t * timer)
{
    wifi_ui_t * ui = lv_timer_get_user_data(timer);
    lv_label_set_text(ui->status, connected ? "Connected" : scanning ? "Scanning…" : running && config.sta.ssid[0] ? "Connecting…" : "Not connected");
}
static void ui_focus(lv_event_t * e)
{
    wifi_ui_t * ui = lv_event_get_user_data(e);
    lv_keyboard_set_textarea(ui->keyboard, lv_event_get_target_obj(e)); ui_keyboard_show(ui->keyboard);
}
static void ui_connect(lv_event_t * e)
{
    wifi_ui_t * ui = lv_event_get_user_data(e);
    char command[96];
    snprintf(command, sizeof(command), "wifi ssid %s", lv_textarea_get_text(ui->ssid)); s3_wifi_command(command);
    snprintf(command, sizeof(command), "wifi password %s", lv_textarea_get_text(ui->password)); s3_wifi_command(command);
    memset(command, 0, sizeof(command));
    ui_keyboard_hide(ui->keyboard); s3_wifi_command("wifi connect");
}
static void ui_delete(lv_event_t * e) { free(lv_event_get_user_data(e)); }
void s3_wifi_settings(lv_obj_t * body)
{
    wifi_ui_t * ui = calloc(1, sizeof(*ui));
    if(!ui) return;
    ui_label(body, "Network", LV_FONT_DEFAULT, UI_DIM);
    ui->ssid = lv_textarea_create(body);
    lv_textarea_set_one_line(ui->ssid, true); lv_textarea_set_max_length(ui->ssid, 32);
    lv_textarea_set_placeholder_text(ui->ssid, "Network name");
    lv_obj_set_size(ui->ssid, LV_PCT(100), 17);
    ui_label(body, "Password", LV_FONT_DEFAULT, UI_DIM);
    ui->password = lv_textarea_create(body);
    lv_textarea_set_one_line(ui->password, true); lv_textarea_set_password_mode(ui->password, true);
    lv_textarea_set_max_length(ui->password, 63); lv_obj_set_size(ui->password, LV_PCT(100), 17);
    ui->keyboard = ui_keyboard(lv_obj_get_parent(body), ui->ssid);
    lv_obj_add_event_cb(ui->ssid, ui_focus, LV_EVENT_FOCUSED, ui);
    lv_obj_add_event_cb(ui->password, ui_focus, LV_EVENT_FOCUSED, ui);
    lv_obj_t * button = ui_button(body, "Join", UI_BLUE, ui_connect, ui);
    lv_obj_set_size(button, LV_PCT(100), 18);
    ui->status = ui_label(body, "Not connected", LV_FONT_DEFAULT, UI_DIM);
    sdk_timer(body, ui_status, 500, ui);
    lv_obj_add_event_cb(body, ui_delete, LV_EVENT_DELETE, ui);
}
