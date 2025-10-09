#include "lora_sf_screen.h"
#include "lora_driver.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "ui_helpers.h"

extern u8g2_t u8g2;

static int selected_item = 0;
static int scroll_offset = 0;

#define VIEWPORT_SIZE 4

static const uint8_t sf_values[] = {6, 7, 8, 9, 10, 11, 12};
static const int sf_count = 7;

void lora_sf_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "SPREADING FACTOR");
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
    
    for (int i = 0; i < VIEWPORT_SIZE && (scroll_offset + i) < sf_count; i++) {
        int item_idx = scroll_offset + i;
        int item_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + (item_height / 2) + 3;
        
        if (item_idx == selected_item) {
            int bar_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + 1;
            int bar_height = item_height - 2;
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }
        
        char sf_str[8];
        snprintf(sf_str, sizeof(sf_str), "SF%d", sf_values[item_idx]);
        
        if (sf_values[item_idx] == config.spreading_factor) {
            int icon_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + (item_height / 2) - (checkmark_height / 2);
            u8g2_DrawXBM(&u8g2, 4, icon_y, checkmark_width, checkmark_height, checkmark_bits);
            u8g2_DrawStr(&u8g2, 16, item_y, sf_str);
        } else {
            u8g2_DrawStr(&u8g2, 16, item_y, sf_str);
        }
        
        if (item_idx == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }
    
    // Footer with one-button UI icons
    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);
    
    u8g2_SendBuffer(&u8g2);
}

void lora_sf_screen_navigate(menu_direction_t direction) {
    if (direction == MENU_DOWN) {
        selected_item = (selected_item + 1) % sf_count;
    } else if (direction == MENU_UP) {
        selected_item = (selected_item - 1 + sf_count) % sf_count;
    }
}

void lora_sf_screen_select(void) {
    lora_config_t config;
    lora_get_config(&config);
    config.spreading_factor = sf_values[selected_item];
    lora_set_config(&config);
}
