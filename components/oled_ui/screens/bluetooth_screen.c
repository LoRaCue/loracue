#include "bluetooth_screen.h"
#include "device_config.h"
#include "oled_ui.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"

#ifndef SIMULATOR_BUILD
#include "bluetooth_config.h"
#endif

extern u8g2_t u8g2;

static int selected_item = 0;

static const char *bt_items[] = {"ON", "OFF"};
static const int bt_item_count = 2;

void bluetooth_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "BLUETOOTH");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);

    // Get current config
    device_config_t config;
    device_config_get(&config);

    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height     = viewport_height / bt_item_count;
    const int bar_height      = (viewport_height / 2) - 1;

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    for (int i = 0; i < bt_item_count; i++) {
        int item_y_start = SEPARATOR_Y_TOP + (i * item_height);
        int bar_y_center = item_y_start + (item_height / 2);
        int bar_y        = bar_y_center - (bar_height / 2);
        int adjusted_bar_height = bar_height;

        if (i == 0) {
            bar_y += 1;
        }
        if (i == 1) {
            adjusted_bar_height -= 1;
        }

        if (i == selected_item) {
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, adjusted_bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }

        int lightbar_center = bar_y + (adjusted_bar_height / 2);
        int text_y          = lightbar_center + 3;

        // Show checkmark for active state
        if ((i == 0 && config.bluetooth_enabled) || (i == 1 && !config.bluetooth_enabled)) {
            int icon_y = lightbar_center - (checkmark_height / 2);
            u8g2_DrawXBM(&u8g2, 4, icon_y, checkmark_width, checkmark_height, checkmark_bits);
            u8g2_DrawStr(&u8g2, 16, text_y, bt_items[i]);
        } else {
            u8g2_DrawStr(&u8g2, 16, text_y, bt_items[i]);
        }

        if (i == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }

    // Footer - show connection status or pairing passkey
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
    
    if (config.bluetooth_enabled) {
#ifndef SIMULATOR_BUILD
        uint32_t passkey;
        if (bluetooth_config_get_passkey(&passkey)) {
            // Show pairing passkey
            char passkey_str[32];
            snprintf(passkey_str, sizeof(passkey_str), "PIN: %06" PRIu32, passkey);
            u8g2_DrawStr(&u8g2, 2, 62, passkey_str);
        } else if (bluetooth_config_is_connected()) {
            u8g2_DrawStr(&u8g2, 2, 62, "Connected");
        } else {
            u8g2_DrawStr(&u8g2, 2, 62, "Advertising...");
        }
#else
        u8g2_DrawStr(&u8g2, 2, 62, "Sim: N/A");
#endif
    } else {
        u8g2_DrawStr(&u8g2, 2, 62, "Disabled");
    }

    u8g2_DrawStr(&u8g2, DISPLAY_WIDTH - 30, 62, "BACK");

    u8g2_SendBuffer(&u8g2);
}

void bluetooth_screen_handle_input(int button)
{
    if (button == 0) { // UP
        selected_item = (selected_item - 1 + bt_item_count) % bt_item_count;
        bluetooth_screen_draw();
    } else if (button == 1) { // DOWN
        selected_item = (selected_item + 1) % bt_item_count;
        bluetooth_screen_draw();
    } else if (button == 2) { // SELECT
        device_config_t config;
        device_config_get(&config);
        
        // Toggle bluetooth based on selection
        config.bluetooth_enabled = (selected_item == 0);
        device_config_set(&config);
        
        // Apply immediately
#ifndef SIMULATOR_BUILD
        bluetooth_config_set_enabled(config.bluetooth_enabled);
#endif
        
        bluetooth_screen_draw();
    }
}
