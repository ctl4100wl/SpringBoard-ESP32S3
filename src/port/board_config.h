#ifndef S3_BOARD_CONFIG_H
#define S3_BOARD_CONFIG_H
/* User-supplied ST7735 Black Tab pinout, portrait. */
#define S3_WIDTH 128
#define S3_HEIGHT 160
#define S3_SCALE 1
#ifndef S3_DEFAULT_BLUR
#define S3_DEFAULT_BLUR 0
#endif
#define S3_LOGICAL_WIDTH (S3_WIDTH * S3_SCALE)
#define S3_LOGICAL_HEIGHT (S3_HEIGHT * S3_SCALE)
/* Portrait throughout: x=0..127, y=0..159. */
#define S3_BUFFER_ROWS 16
#ifndef S3_PANEL_ENABLED
#define S3_PANEL_ENABLED 1
#endif
#ifndef S3_BOOT_GPIO
#define S3_BOOT_GPIO 0
#endif
#ifndef S3_LCD_MOSI
#define S3_LCD_MOSI 11
#define S3_LCD_MISO 13 /* Never GPIO20: reserved away from this display bus. */
#define S3_LCD_CLK 12
#define S3_LCD_CS 10
#define S3_LCD_DC 9
#define S3_LCD_RST 8
#define S3_LCD_BACKLIGHT -1
#endif
/* Stable user-selected write clock; SPI_DMA_CH_AUTO is used. */
#ifndef S3_LCD_HZ
#define S3_LCD_HZ 40000000
#endif
#define S3_LCD_X_GAP 0
#define S3_LCD_Y_GAP 0
#define S3_LCD_INVERT 0
#endif
