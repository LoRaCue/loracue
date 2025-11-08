#include "lcd_ui.h"
#include "lcd_driver.h"
#include "pico/stdlib.h"
#include <stdio.h>

static char buffer[32];

void lcd_ui_init(void) {
    lcd_init();
    
    // Test: fill screen with red to verify LCD works
    lcd_clear(COLOR_RED);
    sleep_ms(1000);
    
    lcd_clear(COLOR_BLACK);
    
    // Title
    lcd_draw_string(60, 5, "I2Console", COLOR_CYAN, COLOR_BLACK);
    
    // Static labels
    lcd_draw_string(5, 30, "Addr:", COLOR_WHITE, COLOR_BLACK);
    lcd_draw_string(5, 50, "TX:", COLOR_WHITE, COLOR_BLACK);
    lcd_draw_string(5, 70, "RX:", COLOR_WHITE, COLOR_BLACK);
    lcd_draw_string(5, 90, "USB:", COLOR_WHITE, COLOR_BLACK);
    lcd_draw_string(5, 110, "Err:", COLOR_WHITE, COLOR_BLACK);
}

void lcd_ui_update(uint8_t i2c_addr, uint16_t tx_avail, uint16_t rx_avail,
                   uint32_t tx_bytes, uint32_t rx_bytes, bool usb_connected,
                   uint32_t errors) {
    
    // I2C Address
    snprintf(buffer, sizeof(buffer), "0x%02X ", i2c_addr);
    lcd_draw_string(80, 30, buffer, COLOR_GREEN, COLOR_BLACK);
    
    // TX Buffer
    snprintf(buffer, sizeof(buffer), "%3d ", tx_avail);
    lcd_draw_string(50, 50, buffer, tx_avail > 200 ? COLOR_RED : COLOR_GREEN, COLOR_BLACK);
    
    // RX Buffer  
    snprintf(buffer, sizeof(buffer), "%4d ", rx_avail);
    lcd_draw_string(50, 70, buffer, rx_avail > 900 ? COLOR_RED : COLOR_GREEN, COLOR_BLACK);
    
    // USB Status
    lcd_draw_string(70, 90, usb_connected ? "CONN " : "DISC ", 
                    usb_connected ? COLOR_GREEN : COLOR_RED, COLOR_BLACK);
    
    // Errors
    snprintf(buffer, sizeof(buffer), "%lu   ", errors);
    lcd_draw_string(70, 110, buffer, errors > 0 ? COLOR_RED : COLOR_GREEN, COLOR_BLACK);
}
