#include "device_mode_screen.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "general_config.h"
#include "system_events.h"
#include "ui_mini.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_icons.h"

extern u8g2_t u8g2;

static int selected_item = 0;

static const char *mode_items[] = {"PRESENTER", "PC"};

static const int mode_item_count = sizeof(mode_items) / sizeof(mode_items[0]);

void device_mode_screen_draw(void)
{
    u8g2_ClearBuffer(&u8g2);

    // Header
    ui_draw_header("DEVICE MODE");

    // Get current mode from persistent config
    general_config_t config;
    general_config_get(&config);

    // Viewport height: 54 - 10 = 44px, each item gets 44/2 = 22px
    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height     = viewport_height / mode_item_count;
    const int bar_height      = (viewport_height / 2) - 1; // 50% of viewport height minus 1px

    // Mode selection
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    for (int i = 0; i < mode_item_count; i++) {
        int item_y_start = SEPARATOR_Y_TOP + 2 + (i * item_height);
        int bar_y_center = item_y_start + (item_height / 2);

        // Calculate lightbar position
        int bar_y               = bar_y_center - (bar_height / 2);
        int adjusted_bar_height = bar_height;

        // Adjust positions: top bar +1px, bottom bar -1px at end
        if (i == 0) {
            bar_y += 1; // Top bar starts 1px down
        }
        if (i == 1) {
            adjusted_bar_height -= 1; // Bottom bar stops 1px up
        }

        if (i == selected_item) {
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, adjusted_bar_height);
            u8g2_SetDrawColor(&u8g2, 0); // Invert color for text
        }

        // Center text and icon relative to the actual lightbar position
        int lightbar_center = bar_y + (adjusted_bar_height / 2);
        int text_y          = lightbar_center + 3; // Adjust for font baseline

        // Show checkmark for active mode
        if ((i == 0 && config.device_mode == DEVICE_MODE_PRESENTER) ||
            (i == 1 && config.device_mode == DEVICE_MODE_PC)) {
            int icon_y = lightbar_center - (checkmark_height / 2);
            u8g2_DrawXBM(&u8g2, 4, icon_y, checkmark_width, checkmark_height, checkmark_bits);
            u8g2_DrawStr(&u8g2, 16, text_y, mode_items[i]);
        } else {
            u8g2_DrawStr(&u8g2, 16, text_y, mode_items[i]);
        }

        if (i == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1); // Reset color
        }
    }

    // Footer with one-button UI icons
    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);

    u8g2_SendBuffer(&u8g2);
}

void device_mode_screen_navigate(menu_direction_t direction)
{
    switch (direction) {
        case MENU_UP:
            selected_item = (selected_item - 1 + mode_item_count) % mode_item_count;
            break;
        case MENU_DOWN:
            selected_item = (selected_item + 1) % mode_item_count;
            break;
    }
}

void device_mode_screen_select(void)
{
    // Get current config
    general_config_t config;
    general_config_get(&config);

    // Update mode based on selection
    device_mode_t new_mode;
    if (selected_item == 0) {
        new_mode = DEVICE_MODE_PRESENTER;
    } else {
        new_mode = DEVICE_MODE_PC;
    }

    // Only proceed if mode actually changed
    if (new_mode == config.device_mode) {
        return; // No change, stay in screen
    }

    // Save to NVS first
    config.device_mode = new_mode;
    general_config_set(&config);

    // Post mode change event
    system_events_post_mode_changed(new_mode);

    // Return to main screen to show new mode
    extern void ui_screen_controller_set(ui_mini_screen_t screen, const ui_status_t *status);
    ui_screen_controller_set(OLED_SCREEN_MAIN, NULL);
}

device_mode_t device_mode_get_current(void)
{
    general_config_t config;
    general_config_get(&config);
    return config.device_mode;
}

void device_mode_screen_reset(void)
{
    selected_item = 0;
}
