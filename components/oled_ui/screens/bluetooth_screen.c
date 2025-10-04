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

    // Footer with navigation (like device mode screen)
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    // Left: Back arrow
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, 64, "Back");

    // Middle: Next arrow
    u8g2_DrawXBM(&u8g2, 40, 56, track_next_width, track_next_height, track_next_bits);
    u8g2_DrawStr(&u8g2, 46, 64, "Next");

    // Right: Select with both buttons icon
    const char *select_text = "Select";
    int select_text_width   = u8g2_GetStrWidth(&u8g2, select_text);
    int select_x            = DISPLAY_WIDTH - both_buttons_width - select_text_width - 2;
    u8g2_DrawXBM(&u8g2, select_x, 56, both_buttons_width, both_buttons_height, both_buttons_bits);
    u8g2_DrawStr(&u8g2, select_x + both_buttons_width + 2, 64, select_text);

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
