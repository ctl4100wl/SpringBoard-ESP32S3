/* ST7735R Black Tab, RGB565, RGB order, portrait, zero column/row offsets.
 * Command register values checked against TFT_eSPI's ST7735R initialization:
 * https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_Drivers/ST7735_Init.h
 * Uses native ESP-IDF panel IO; no Arduino/TFT_eSPI runtime dependency. */
#include "st7735.h"
#include "board_config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define SEND(cmd, data, len) do { esp_err_t error = esp_lcd_panel_io_tx_param(io, cmd, data, len); if(error != ESP_OK) return error; } while(0)

esp_err_t s3_st7735_init(esp_lcd_panel_io_handle_t io, int reset_gpio)
{
    if(reset_gpio >= 0) {
        gpio_config_t config = { .pin_bit_mask = 1ULL << reset_gpio, .mode = GPIO_MODE_OUTPUT };
        esp_err_t error = gpio_config(&config);
        if(error != ESP_OK) return error;
        gpio_set_level(reset_gpio, 1); vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(reset_gpio, 0); vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(reset_gpio, 1); vTaskDelay(pdMS_TO_TICKS(120));
    }
    SEND(0x01, NULL, 0); vTaskDelay(pdMS_TO_TICKS(150));
    SEND(0x11, NULL, 0); vTaskDelay(pdMS_TO_TICKS(500));
    const uint8_t frame[] = {1, 0x2C, 0x2D};
    const uint8_t partial[] = {1, 0x2C, 0x2D, 1, 0x2C, 0x2D};
    SEND(0xB1, frame, sizeof(frame)); SEND(0xB2, frame, sizeof(frame));
    SEND(0xB3, partial, sizeof(partial));
    const uint8_t inversion = 7; SEND(0xB4, &inversion, 1);
    const uint8_t power1[] = {0xA2, 2, 0x84}; SEND(0xC0, power1, sizeof(power1));
    const uint8_t power2 = 0xC5; SEND(0xC1, &power2, 1);
    const uint8_t power3[] = {0x0A, 0}; SEND(0xC2, power3, sizeof(power3));
    const uint8_t power4[] = {0x8A, 0x2A}; SEND(0xC3, power4, sizeof(power4));
    const uint8_t power5[] = {0x8A, 0xEE}; SEND(0xC4, power5, sizeof(power5));
    const uint8_t vcom = 0x0E; SEND(0xC5, &vcom, 1);
    SEND(S3_LCD_INVERT ? 0x21 : 0x20, NULL, 0);
    const uint8_t access = 0xC0; /* MX|MY, no XY swap, RGB (BGR bit clear). */
    SEND(0x36, &access, 1);
    const uint8_t format = 5; SEND(0x3A, &format, 1);
    const uint8_t positive[] = {2, 0x1C, 7, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0, 1, 3, 0x10};
    const uint8_t negative[] = {3, 0x1D, 7, 6, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0, 0, 2, 0x10};
    SEND(0xE0, positive, sizeof(positive)); SEND(0xE1, negative, sizeof(negative));
    SEND(0x13, NULL, 0); vTaskDelay(pdMS_TO_TICKS(10));
    SEND(0x29, NULL, 0); vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

esp_err_t s3_st7735_draw(esp_lcd_panel_io_handle_t io, int x1, int y1, int x2, int y2, const void * pixels)
{
    static int previous_x1 = -1, previous_x2 = -1, previous_y1 = -1, previous_y2 = -1;
    if(!pixels || x1 < 0 || y1 < 0 || x2 > S3_WIDTH || y2 > S3_HEIGHT || x2 <= x1 || y2 <= y1)
        return ESP_ERR_INVALID_ARG;
    size_t bytes = (size_t)(x2 - x1) * (y2 - y1) * sizeof(uint16_t);
    x1 += S3_LCD_X_GAP; x2 += S3_LCD_X_GAP;
    y1 += S3_LCD_Y_GAP; y2 += S3_LCD_Y_GAP;
    uint8_t column[] = {x1 >> 8, x1 & 255, (x2 - 1) >> 8, (x2 - 1) & 255};
    uint8_t row[] = {y1 >> 8, y1 & 255, (y2 - 1) >> 8, (y2 - 1) & 255};
    if(x1 != previous_x1 || x2 != previous_x2) {
        SEND(0x2A, column, sizeof(column)); previous_x1 = x1; previous_x2 = x2;
    }
    if(y1 != previous_y1 || y2 != previous_y2) {
        SEND(0x2B, row, sizeof(row)); previous_y1 = y1; previous_y2 = y2;
    }
    return esp_lcd_panel_io_tx_color(io, 0x2C, pixels, bytes);
}
