#include "lora_txpower_screen.h"
#include "lora_driver.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "ui_helpers.h"

extern u8g2_t u8g2;

static int selected_item = 0;
static int scroll_offset = 0;

#define VIEWPORT_SIZE 4

// TX power range: 5 to 20 dBm
static const int8_t txpower_values[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
static const int txpower_count = 16;

void lora_txpower_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "TX POWER");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
    
    // Get current config
    lora_config_t config;
    lora_get_config(&config);
    
    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height = viewport_height / VIEWPORT_SIZE;
    
    // Adjust scroll
    if (selected_item < scroll_offset) {
        scroll_offset = selected_item;
    } else if (selected_item >= scroll_offset + VIEWPORT_SIZE) {
        scroll_offset = selected_item - VIEWPORT_SIZE + 1;
    }
    
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    for (int i = 0; i < VIEWPORT_SIZE && (scroll_offset + i) < txpower_count; i++) {
        int item_idx = scroll_offset + i;
        int item_y = SEPARATOR_Y_TOP + (i * item_height) + (item_height / 2) + 3;
        
        if (item_idx == selected_item) {
            int bar_y = SEPARATOR_Y_TOP + (i * item_height) + 1;
            int bar_height = item_height - 2;
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }
        
        char power_str[16];
        snprintf(power_str, sizeof(power_str), "%d dBm", txpower_values[item_idx]);
        
        if (txpower_values[item_idx] == config.tx_power) {
            int icon_y = SEPARATOR_Y_TOP + (i * item_height) + (item_height / 2) - (checkmark_height / 2);
            u8g2_DrawXBM(&u8g2, 4, icon_y, checkmark_width, checkmark_height, checkmark_bits);
            u8g2_DrawStr(&u8g2, 16, item_y, power_str);
        } else {
            u8g2_DrawStr(&u8g2, 16, item_y, power_str);
        }
        
        if (item_idx == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }
    
    // Footer with one-button UI icons
    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);
    
    u8g2_SendBuffer(&u8g2);
}

void lora_txpower_screen_navigate(menu_direction_t direction) {
    if (direction == MENU_DOWN) {
        selected_item = (selected_item + 1) % txpower_count;
    } else if (direction == MENU_UP) {
        selected_item = (selected_item - 1 + txpower_count) % txpower_count;
    }
}

void lora_txpower_screen_select(void) {
    lora_config_t config;
    lora_get_config(&config);
    config.tx_power = txpower_values[selected_item];
    lora_set_config(&config);
}
