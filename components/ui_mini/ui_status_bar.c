#include "ui_status_bar.h"
#include "icons/ui_status_icons.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "esp_log.h"
#include <string.h>

extern u8g2_t u8g2;

#define LONG_PRESS_ICON_WIDTH 13
#define LONG_PRESS_ICON_HEIGHT 7

static ui_status_t last_status = {0};
static bool first_draw = true;

void ui_status_bar_draw(const ui_status_t *status)
{
    // Store current status for next comparison
    memcpy(&last_status, status, sizeof(ui_status_t));
    first_draw = false;
    
    // Draw brand name
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, TEXT_MARGIN_LEFT - 1, 8, "LORACUE");

    // Calculate dynamic icon positions (right to left, no gaps)
    // RF and Battery are fixed on the right
    // USB and Bluetooth fill in from the right when active
    int next_x = RF_ICON_X;

    // Count active dynamic icons
    int active_count = 0;
    if (status->bluetooth_enabled)
        active_count++;
    if (status->usb_connected)
        active_count++;

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

    // Menu hint on right with long press icon
    int suffix_width = u8g2_GetStrWidth(&u8g2, " Menu");
    int total_width  = LONG_PRESS_ICON_WIDTH + suffix_width;

    int start_x = DISPLAY_WIDTH - total_width - TEXT_MARGIN_RIGHT;
    ui_button_long_draw_at(start_x, DISPLAY_HEIGHT - LONG_PRESS_ICON_HEIGHT - 1);
    u8g2_DrawStr(&u8g2, start_x + LONG_PRESS_ICON_WIDTH, DISPLAY_HEIGHT - 1, " Menu");
}

void ui_status_bar_reset(void)
{
    first_draw = true;
}
