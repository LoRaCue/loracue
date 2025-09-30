#include "ui_status_bar.h"
#include "ui_config.h"
#include "icons/ui_battery.h"
#include "icons/ui_usb.h"
#include "icons/ui_rf.h"
#include "u8g2.h"
#include <string.h>

extern u8g2_t u8g2;

void ui_status_bar_draw(const ui_status_t* status) {
    // Draw brand name
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, TEXT_MARGIN_LEFT - 1, 8, "LORACUE");
    
    // Draw status icons
    ui_usb_draw(status->usb_connected);
    ui_rf_draw(status->signal_strength);
    ui_battery_draw(status->battery_level);
    
    // Draw separator line
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
}

void ui_bottom_bar_draw(const ui_status_t* status) {
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    // Device name on left
    u8g2_DrawStr(&u8g2, TEXT_MARGIN_LEFT - 1, DISPLAY_HEIGHT - 1, status->device_name);
    
    // Menu hint on right
    const char* menu_text = "3s [<+>] Menu";
    int menu_width = u8g2_GetStrWidth(&u8g2, menu_text);
    u8g2_DrawStr(&u8g2, DISPLAY_WIDTH - menu_width - TEXT_MARGIN_RIGHT, DISPLAY_HEIGHT - 1, menu_text);
}
