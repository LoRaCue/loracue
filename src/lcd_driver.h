#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdint.h>

#define LCD_WIDTH 240
#define LCD_HEIGHT 135

#define LCD_DC_PIN 8
#define LCD_CS_PIN 9
#define LCD_CLK_PIN 10
#define LCD_MOSI_PIN 11
#define LCD_RST_PIN 12
#define LCD_BL_PIN 13

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_CYAN 0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW 0xFFE0
#define COLOR_GRAY 0x8410
#define COLOR_DARKGRAY 0x2104

void lcd_init(void);
void lcd_clear(uint16_t color);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);
void lcd_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);
void lcd_set_backlight(uint8_t brightness);

#endif
