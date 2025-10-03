#include "device_mode_screen.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "device_config.h"
#include "u8g2.h"

extern u8g2_t u8g2;

static int selected_item = 0;

static const char* mode_items[] = {
    "PRESENTER",
    "PC"
};

static const int mode_item_count = sizeof(mode_items) / sizeof(mode_items[0]);

void device_mode_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "DEVICE MODE");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
    
    // Get current mode from persistent config
    device_config_t config;
    device_config_get(&config);
    
    // Mode selection
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    for (int i = 0; i < mode_item_count; i++) {
        int y = 20 + (i * 15);
        
        if (i == selected_item) {
            // Highlight selected item (moved up 1px)
            u8g2_DrawBox(&u8g2, 0, y - 9, DISPLAY_WIDTH, 12);
            u8g2_SetDrawColor(&u8g2, 0);  // Invert color for text
            
            // Show checkmark for active mode
            if ((i == 0 && config.device_mode == DEVICE_MODE_PRESENTER) || 
                (i == 1 && config.device_mode == DEVICE_MODE_PC)) {
                u8g2_DrawXBM(&u8g2, 4, y - 7, checkmark_width, checkmark_height, checkmark_bits);
                u8g2_DrawStr(&u8g2, 16, y, mode_items[i]);  // Adjusted for 9px width + margin
            } else {
                u8g2_DrawStr(&u8g2, 16, y, mode_items[i]);
            }
            
            u8g2_SetDrawColor(&u8g2, 1);  // Reset color
        } else {
            // Show checkmark for active mode
            if ((i == 0 && config.device_mode == DEVICE_MODE_PRESENTER) || 
                (i == 1 && config.device_mode == DEVICE_MODE_PC)) {
                u8g2_DrawXBM(&u8g2, 4, y - 7, checkmark_width, checkmark_height, checkmark_bits);
                u8g2_DrawStr(&u8g2, 16, y, mode_items[i]);  // Adjusted for 9px width + margin
            } else {
                u8g2_DrawStr(&u8g2, 16, y, mode_items[i]);
            }
        }
    }
    
    // Footer with navigation (like menu screen)
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    // Left: Back arrow
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, 64, "Back");
    
    // Middle: Next arrow
    u8g2_DrawXBM(&u8g2, 40, 56, track_next_width, track_next_height, track_next_bits);
    u8g2_DrawStr(&u8g2, 46, 64, "Next");
    
    // Right: Select with both buttons icon
    const char* select_text = "Select";
    int select_text_width = u8g2_GetStrWidth(&u8g2, select_text);
    int select_x = DISPLAY_WIDTH - both_buttons_width - select_text_width - 2;
    u8g2_DrawXBM(&u8g2, select_x, 56, both_buttons_width, both_buttons_height, both_buttons_bits);
    u8g2_DrawStr(&u8g2, select_x + both_buttons_width + 2, 64, select_text);
    
    u8g2_SendBuffer(&u8g2);
}

void device_mode_screen_navigate(menu_direction_t direction) {
    switch (direction) {
        case MENU_UP:
            selected_item = (selected_item - 1 + mode_item_count) % mode_item_count;
            break;
        case MENU_DOWN:
            selected_item = (selected_item + 1) % mode_item_count;
            break;
    }
}

void device_mode_screen_select(void) {
    // Get current config
    device_config_t config;
    device_config_get(&config);
    
    // Update mode based on selection
    if (selected_item == 0) {
        config.device_mode = DEVICE_MODE_PRESENTER;
    } else if (selected_item == 1) {
        config.device_mode = DEVICE_MODE_PC;
    }
    
    // Save to NVS
    device_config_set(&config);
    
    // Trigger mode change logic in main.c
    extern void check_device_mode_change(void);
    check_device_mode_change();
}

device_mode_t device_mode_get_current(void) {
    device_config_t config;
    device_config_get(&config);
    return config.device_mode;
}

void device_mode_screen_reset(void) {
    selected_item = 0;
}
