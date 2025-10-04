#include "ui_status_bar.h"
#include "icons/ui_status_icons.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include <string.h>

extern u8g2_t u8g2;

void ui_status_bar_draw(const ui_status_t *status)
{
    // Draw brand name
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, TEXT_MARGIN_LEFT - 1, 8, "LORACUE");

    // Calculate dynamic icon positions (right to left, no gaps)
    // RF and Battery are fixed on the right
    // USB and Bluetooth fill in from the right when active
    int next_x = RF_ICON_X;
    
    // Count active dynamic icons
    int active_count = 0;
    if (status->bluetooth_enabled) active_count++;
    if (status->usb_connected) active_count++;
    
    // Calculate starting position for dynamic icons
    if (active_count > 0) {
        next_x = RF_ICON_X - ICON_SPACING;
        if (status->bluetooth_enabled) {
            next_x -= BT_ICON_WIDTH;
        }
        if (status->usb_connected) {
            next_x -= USB_ICON_WIDTH;
            if (status->bluetooth_enabled) {
                next_x -= ICON_SPACING;
            }
        }
    }
    
    // Draw icons left to right: USB, Bluetooth, RF, Battery
    if (status->usb_connected) {
        ui_usb_draw_at(next_x, 0);
        next_x += USB_ICON_WIDTH + ICON_SPACING;
    }
    
    if (status->bluetooth_enabled) {
        ui_bluetooth_draw_at(next_x, 0, status->bluetooth_connected);
        next_x += BT_ICON_WIDTH + ICON_SPACING;
    }
    
    // Fixed positions for RF and Battery
    ui_rf_draw(status->signal_strength);
    ui_battery_draw(status->battery_level);

    // Draw separator line
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
}

void ui_bottom_bar_draw(const ui_status_t *status)
{
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    // Device name on left
    u8g2_DrawStr(&u8g2, TEXT_MARGIN_LEFT - 1, DISPLAY_HEIGHT - 1, status->device_name);

    // Menu hint on right with icon
    const char *menu_text   = "3s ";
    const char *menu_suffix = " Menu";
    int text_width          = u8g2_GetStrWidth(&u8g2, menu_text);
    int suffix_width        = u8g2_GetStrWidth(&u8g2, menu_suffix);
    int total_width         = text_width + both_buttons_width + suffix_width;

    int start_x = DISPLAY_WIDTH - total_width - TEXT_MARGIN_RIGHT;
    u8g2_DrawStr(&u8g2, start_x, DISPLAY_HEIGHT - 1, menu_text);
    u8g2_DrawXBM(&u8g2, start_x + text_width, DISPLAY_HEIGHT - both_buttons_height - 1, both_buttons_width,
                 both_buttons_height, both_buttons_bits);
    u8g2_DrawStr(&u8g2, start_x + text_width + both_buttons_width, DISPLAY_HEIGHT - 1, menu_suffix);
}
