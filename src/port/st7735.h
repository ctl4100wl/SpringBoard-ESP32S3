#ifndef S3_ST7735_H
#define S3_ST7735_H
#include "esp_lcd_panel_io.h"
esp_err_t s3_st7735_init(esp_lcd_panel_io_handle_t io, int reset_gpio);
esp_err_t s3_st7735_draw(esp_lcd_panel_io_handle_t io, int x1, int y1, int x2, int y2, const void * pixels);
#endif
