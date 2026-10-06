#define S3_NATIVE_DRIVER 1
#include "render_variant.h"
#include "s3_wifi.h"
#include "hardware.h"
#include "src/os/sdk.h"
#include "board_config.h"
#include "st7735.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "driver/usb_serial_jtag.h"
#include <string.h>

static const char * TAG = "s3_hw";
static unsigned flushes;
unsigned s3_flush_count(void) { return flushes; }
static uint32_t tick_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

typedef struct { uint16_t * pixels; lv_area_t dirty; bool moving; } frame_job_t;
static QueueHandle_t frames;
static SemaphoreHandle_t source_done;
static bool source_pending;
static uint16_t * logical[2];
static uint16_t * displayed;
static bool tiled=false, have_damage, automatic_render=true;
static lv_area_t damage;
static bool quality = true, perf_logging;
static uint32_t perf_next;
static portMUX_TYPE stats_lock = portMUX_INITIALIZER_UNLOCKED;
static struct { uint32_t sent, scaled, ui, ui_max_us, ui_hist[65], motion_intervals; uint64_t tx_us, scale_us, ui_us, motion_us; int64_t started, sending, last_sent; bool sending_motion, previous_motion; } stats;
static int64_t ui_started;
bool s3_profile_enabled;
static struct { uint64_t us; unsigned calls; } draw_profile[32];
void s3_draw_profile(unsigned type,int64_t elapsed)
{
    if(type<32) { draw_profile[type].us+=elapsed; draw_profile[type].calls++; }
}
void s3_profile(bool on)
{
    s3_profile_enabled=on;
    if(on) memset(draw_profile,0,sizeof(draw_profile));
    else for(unsigned i=0;i<32;i++) if(draw_profile[i].calls)
        ESP_LOGI(TAG,"DRAW type=%u calls=%u total=%.2fms average=%.2fms",i,draw_profile[i].calls,draw_profile[i].us/1000.0,draw_profile[i].us/(1000.0*draw_profile[i].calls));
}
#if S3_PANEL_ENABLED
static esp_lcd_panel_io_handle_t panel_io;
static SemaphoreHandle_t flush_done;
static bool transfer_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t * event, void * context)
{
    (void)io; (void)event; (void)context;
    int64_t now = esp_timer_get_time();
    portENTER_CRITICAL_ISR(&stats_lock);
    stats.sent++; stats.tx_us += now - stats.sending;
    if(stats.sending_motion && stats.previous_motion) {
        stats.motion_intervals++; stats.motion_us += now - stats.last_sent;
    }
    stats.last_sent = now; stats.previous_motion = stats.sending_motion;
    portEXIT_CRITICAL_ISR(&stats_lock);
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(flush_done, &wake);
    return wake == pdTRUE;
}
#endif
/* The display receives native RGB565. Swap byte order in the borrowed DMA
 * buffer, then restore it before releasing it to LVGL's direct-buffer sync.
 * Full-width dirty rows are contiguous; no copy or resampling is required. */
static void IRAM_ATTR swap_pixels(uint16_t * p, size_t count)
{
    while(count && ((uintptr_t)p & 3)) { *p=(*p<<8)|(*p>>8); p++; count--; }
    uint32_t * q=(uint32_t *)p;
    while(count>=2) { uint32_t v=*q; *q++=((v&0x00ff00ff)<<8)|((v&0xff00ff00)>>8); count-=2; }
    if(count) { p=(uint16_t *)q; *p=(*p<<8)|(*p>>8); }
}
static void display_worker(void * arg)
{
    (void)arg; frame_job_t job;
    while(true) {
        xQueueReceive(frames,&job,portMAX_DELAY);
        uint16_t * rows=job.pixels+job.dirty.y1*S3_WIDTH;
        size_t count=(job.dirty.y2-job.dirty.y1+1)*S3_WIDTH;
        int64_t start=esp_timer_get_time();
        swap_pixels(rows,count);
        portENTER_CRITICAL(&stats_lock);
        stats.scaled++; stats.scale_us+=esp_timer_get_time()-start;
        portEXIT_CRITICAL(&stats_lock);
#if S3_PANEL_ENABLED
        portENTER_CRITICAL(&stats_lock); stats.sending=esp_timer_get_time(); stats.sending_motion=job.moving; portEXIT_CRITICAL(&stats_lock);
        ESP_ERROR_CHECK(s3_st7735_draw(panel_io,0,job.dirty.y1,S3_WIDTH,job.dirty.y2+1,rows));
        if(xSemaphoreTake(flush_done,pdMS_TO_TICKS(2000))!=pdTRUE) abort();
#else
        portENTER_CRITICAL(&stats_lock); stats.sent++; portEXIT_CRITICAL(&stats_lock);
#endif
        swap_pixels(rows,count);
        displayed=job.pixels;
        xSemaphoreGive(source_done);
    }
}
static void wait_source(lv_display_t * display);
static void flush(lv_display_t * display, const lv_area_t * area, uint8_t * pixels)
{
    lv_area_t changed=*area;
    if(have_damage) {
        damage.x1=LV_MIN(damage.x1,changed.x1); damage.y1=LV_MIN(damage.y1,changed.y1);
        damage.x2=LV_MAX(damage.x2,changed.x2); damage.y2=LV_MAX(damage.y2,changed.y2);
    } else { damage=changed; have_damage=true; }
    if(!lv_display_flush_is_last(display)) { lv_display_flush_ready(display); return; }
    flushes++; source_pending = true;
    frame_job_t job = { (uint16_t *)pixels, damage, s3_shell_busy() || fx_any_busy() || lv_anim_count_running() > 0 };
    have_damage=false;
    if(xQueueSend(frames, &job, pdMS_TO_TICKS(2000)) != pdTRUE) abort();
}
static void wait_source(lv_display_t * display)
{
    (void)display;
    if(!source_pending) return;
    /* LVGL clears its flushing flag after this callback returns. No cross-core LVGL calls. */
    if(xSemaphoreTake(source_done, pdMS_TO_TICKS(2000)) != pdTRUE) abort();
    source_pending = false;
}
static void select_render_mode(bool full)
{
    lv_display_t * display = lv_display_get_default();
    lv_display_render_mode_t mode=full ? LV_DISPLAY_RENDER_MODE_FULL : LV_DISPLAY_RENDER_MODE_DIRECT;
    if(lv_display_get_render_mode(display)==mode) return;
    if(source_pending) { wait_source(display); lv_display_flush_ready(display); }
    tiled=false; have_damage=false;
    lv_display_set_buffers(display,logical[0],logical[1],S3_LOGICAL_WIDTH*S3_LOGICAL_HEIGHT*sizeof(uint16_t),full ? LV_DISPLAY_RENDER_MODE_FULL : LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_obj_invalidate(lv_screen_active());
}
void s3_render_mode(bool full) { automatic_render=false; select_render_mode(full); s3_perf_reset(); }
void s3_render_auto(void) { automatic_render=true; }
bool s3_render_is_auto(void) { return automatic_render; }
bool s3_render_is_tiled(void) { return tiled; }
void s3_render_tiles(void)
{
    /* Compatibility command: native direct mode already uses internal SRAM. */
    s3_render_mode(false);
}
static void refresh_event(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_REFR_START) ui_started = esp_timer_get_time();
    else if(lv_event_get_code(e) == LV_EVENT_REFR_READY) {
        portENTER_CRITICAL(&stats_lock);
        uint32_t elapsed = esp_timer_get_time() - ui_started;
        stats.ui++; stats.ui_us += elapsed;
        stats.ui_max_us = LV_MAX(stats.ui_max_us, elapsed);
        stats.ui_hist[LV_MIN(64, (elapsed + 999) / 1000)]++;
        portEXIT_CRITICAL(&stats_lock);
    }
}
void s3_perf_reset(void)
{
    if(source_pending) {
        lv_display_t * display = lv_display_get_default();
        wait_source(display); lv_display_flush_ready(display);
    }
    portENTER_CRITICAL(&stats_lock);
    stats.sent = stats.scaled = stats.ui = 0;
    stats.tx_us = stats.scale_us = stats.ui_us = 0; stats.started = esp_timer_get_time();
    stats.ui_max_us = 0; memset(stats.ui_hist, 0, sizeof(stats.ui_hist));
    stats.motion_intervals = 0; stats.motion_us = 0; stats.previous_motion = false;
    portEXIT_CRITICAL(&stats_lock);
}
extern uint32_t s3_simd_hits;
void s3_perf_report(void)
{
    portENTER_CRITICAL(&stats_lock); typeof(stats) copy = stats; portEXIT_CRITICAL(&stats_lock);
    double seconds = (esp_timer_get_time() - copy.started)/1000000.0;
    unsigned p95 = 0, count = 0;
    for(unsigned i = 0; i <= 64; i++) { count += copy.ui_hist[i]; if(count * 100 >= copy.ui * 95) { p95 = i; break; } }
    ESP_LOGI(TAG, "FRAME UI_p95=%s%ums UI_max=%.2fms blur=%d lens=%d", p95 == 64 ? ">=" : "", p95, copy.ui_max_us/1000.0, ui_blur_enabled(), ui_lens_enabled());
    ESP_LOGI(TAG, "MOTION displayed=%.2f fps intervals=%u active_ms=%.2f", copy.motion_us ? copy.motion_intervals * 1000000.0 / copy.motion_us : 0,
             copy.motion_intervals, copy.motion_us / 1000.0);
    ESP_LOGI(TAG, "PERF displayed=%.2f fps frames=%u UI=%.2fms byte_swap=%.2fms SPI=%.2fms SIMD=%u native=128x160 render=%s heap=%u psram=%u",
        seconds > 0 ? copy.sent/seconds : 0, copy.sent,
        copy.ui ? copy.ui_us/(1000.0*copy.ui) : 0, copy.scaled ? copy.scale_us/(1000.0*copy.scaled) : 0,
        copy.sent ? copy.tx_us/(1000.0*copy.sent) : 0, (unsigned)s3_simd_hits,
        lv_display_get_render_mode(lv_display_get_default())==LV_DISPLAY_RENDER_MODE_FULL ? "full" : "direct", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
void s3_perf_logging(bool enabled) { perf_logging = enabled; perf_next = tick_ms()+5000; s3_perf_reset(); }
void s3_filter(bool filtered) { quality = filtered; }

void s3_screenshot(void)
{
    lv_display_t * d=lv_display_get_default();
    if(source_pending) { wait_source(d); lv_display_flush_ready(d); }
    if(!displayed) return;
    const char header[]="S3FRAME 128 160 40960\n";
    usb_serial_jtag_write_bytes(header,sizeof(header)-1,pdMS_TO_TICKS(2000));
    const uint8_t * data=(const uint8_t *)displayed;
    size_t left=S3_WIDTH*S3_HEIGHT*2;
    while(left) {
        int sent=usb_serial_jtag_write_bytes(data,LV_MIN(left,64),pdMS_TO_TICKS(2000));
        if(sent<=0) return;
        data+=sent; left-=sent;
    }
}

static void display_init(void)
{
    size_t bytes = S3_WIDTH * S3_HEIGHT * sizeof(uint16_t);
    size_t frame_bytes = S3_LOGICAL_WIDTH * S3_LOGICAL_HEIGHT * sizeof(uint16_t);
    void * frame = heap_caps_aligned_alloc(16, frame_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void * second = heap_caps_aligned_alloc(16, frame_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    logical[0]=frame; logical[1]=second;
    if(!frame || !second) { ESP_LOGE(TAG,"Native UI requires two internal 40960-byte DMA buffers"); abort(); }
    ESP_LOGI(TAG,"Native 128x160 raster; two internal DMA buffers, no framebuffer scaling");
    lv_display_t * display = lv_display_create(S3_LOGICAL_WIDTH, S3_LOGICAL_HEIGHT);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, frame, second, frame_bytes, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(display, flush);
    lv_display_set_flush_wait_cb(display, wait_source);
    lv_display_add_event_cb(display, refresh_event, LV_EVENT_REFR_START, NULL);
    lv_display_add_event_cb(display, refresh_event, LV_EVENT_REFR_READY, NULL);
#if S3_PANEL_ENABLED
    if(S3_LCD_MOSI < 0 || S3_LCD_CLK < 0 || S3_LCD_CS < 0 || S3_LCD_DC < 0) {
        ESP_LOGE(TAG, "LCD pins are not configured"); abort();
    }
    flush_done = xSemaphoreCreateBinary();
    if(!flush_done) abort();
    spi_bus_config_t bus = { .mosi_io_num = S3_LCD_MOSI, .miso_io_num = S3_LCD_MISO,
        .sclk_io_num = S3_LCD_CLK, .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = bytes };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    esp_lcd_panel_io_spi_config_t io_config = { .dc_gpio_num = S3_LCD_DC,
        .cs_gpio_num = S3_LCD_CS, .pclk_hz = S3_LCD_HZ, .spi_mode = 0,
        .trans_queue_depth = 2, .lcd_cmd_bits = 8, .lcd_param_bits = 8,
        .on_color_trans_done = transfer_done };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &panel_io));
    ESP_ERROR_CHECK(s3_st7735_init(panel_io, S3_LCD_RST));
    ESP_LOGI(TAG, "ST7735 Black Tab portrait RGB 128x160 write clock=%u", S3_LCD_HZ);
    if(S3_LCD_BACKLIGHT >= 0) {
        gpio_set_direction(S3_LCD_BACKLIGHT, GPIO_MODE_OUTPUT);
        gpio_set_level(S3_LCD_BACKLIGHT, 1);
    }

#else
    ESP_LOGW(TAG, "Headless 128x160 render mode; no display GPIOs driven");
#endif
    source_done = xSemaphoreCreateBinary();
    frames = xQueueCreate(2, sizeof(frame_job_t));
    if(!source_done || !frames || xTaskCreatePinnedToCore(display_worker, "lcd_push", 4096, NULL, 5, NULL, 0) != pdPASS) abort();
    s3_perf_reset();
    tiled=false;
    s3_render_auto();
}

static void ui_task(void * arg)
{
    (void)arg;
    lv_init();
    lv_tick_set_cb(tick_ms);
    lv_group_set_default(lv_group_create());
    display_init();
    ui_effects_set(S3_DEFAULT_BLUR != 0, false);
    s3_input_init();
    shell_init();
    s3_wifi_restore();
    lv_timer_set_period(lv_display_get_refr_timer(lv_display_get_default()), 16);
    gpio_config_t boot = { .pin_bit_mask = 1ULL << S3_BOOT_GPIO,
        .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE };
    ESP_ERROR_CHECK(gpio_config(&boot));
    while(true) {
        uint32_t buttons = 0;
        if(xTaskNotifyWait(0, 1, &buttons, 0) == pdTRUE) {
            if(s3_smoke_running()) s3_smoke_cancel(); else s3_smoke_start();
        }
        uint32_t now = tick_ms();
        s3_command_poll();
        s3_wifi_poll();
        uint32_t delay = lv_timer_handler();
        if(automatic_render) {
            lv_indev_t * input=lv_indev_get_next(NULL);
            bool moving=s3_shell_busy() || fx_any_busy() || lv_anim_count_running()>0 ||
                (input && lv_indev_get_state(input)==LV_INDEV_STATE_PRESSED);
            select_render_mode(moving);
        }
        if(perf_logging && (int32_t)(now-perf_next) >= 0) { s3_perf_report(); s3_perf_reset(); perf_next=now+5000; }
        vTaskDelay(LV_MAX(1, pdMS_TO_TICKS(LV_CLAMP(1, delay, 5))));
    }
}

static void boot_task(void * arg)
{
    TaskHandle_t ui = (TaskHandle_t)arg;
    int previous = gpio_get_level(S3_BOOT_GPIO), stable = previous;
    bool armed = false; uint32_t changed = tick_ms();
    while(true) {
        int value = gpio_get_level(S3_BOOT_GPIO); uint32_t now = tick_ms();
        if(value != previous) { previous = value; changed = now; }
        if(now - changed >= 40) {
            if(value) armed = true;
            if(value != stable) {
                stable = value;
                if(!stable && armed) { armed = false; xTaskNotify(ui, 1, eSetBits); }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void springboard_hardware_start(void)
{
    /* Never erase existing user NVS automatically. */
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(TAG, "Internal heap=%u, PSRAM=%u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    TaskHandle_t ui;
    if(xTaskCreatePinnedToCore(ui_task, "springboard", 16384, NULL, 4, &ui, 1) != pdPASS) abort();
    /* UI configures GPIO before normal user interaction; monitor independently of expensive draws. */
    gpio_set_direction(S3_BOOT_GPIO, GPIO_MODE_INPUT); gpio_set_pull_mode(S3_BOOT_GPIO, GPIO_PULLUP_ONLY);
    if(xTaskCreatePinnedToCore(boot_task, "boot_button", 2048, ui, 3, NULL, 0) != pdPASS) abort();
}
