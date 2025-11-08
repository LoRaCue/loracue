#ifndef LCD_UI_H
#define LCD_UI_H

#include <stdint.h>
#include <stdbool.h>

void lcd_ui_init(void);
void lcd_ui_update(uint8_t i2c_addr, uint16_t tx_avail, uint16_t rx_avail, 
                   uint32_t tx_bytes, uint32_t rx_bytes, bool usb_connected,
                   uint32_t errors);

#endif
