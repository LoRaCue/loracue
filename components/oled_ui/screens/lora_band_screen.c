#include "lora_band_screen.h"
#include "lora_driver.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"

extern u8g2_t u8g2;

static int selected_item = 0;

static const char *band_items[] = {"433 MHz", "868 MHz", "915 MHz"};
static const uint32_t band_frequencies[] = {433000000, 868000000, 915000000};
static const int band_count = 3;

void lora_band_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "BAND");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
    
    // Get current config
    lora_config_t config;
    lora_get_config(&config);
    
    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height = viewport_height / band_count;
    
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    for (int i = 0; i < band_count; i++) {
        int item_y = SEPARATOR_Y_TOP + (i * item_height) + (item_height / 2) + 3;
        
        if (i == selected_item) {
            int bar_y = SEPARATOR_Y_TOP + (i * item_height) + 1;
            int bar_height = item_height - 2;
            if (i == band_count - 1) bar_height -= 1;
            
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }
        
        // Show checkmark for current band
        bool is_current = false;
        if (i == 0 && config.frequency >= 430000000 && config.frequency <= 440000000) is_current = true;
        if (i == 1 && config.frequency >= 863000000 && config.frequency <= 870000000) is_current = true;
        if (i == 2 && config.frequency >= 902000000 && config.frequency <= 928000000) is_current = true;
        
        if (is_current) {
            int icon_y = SEPARATOR_Y_TOP + (i * item_height) + (item_height / 2) - (checkmark_height / 2);
            u8g2_DrawXBM(&u8g2, 4, icon_y, checkmark_width, checkmark_height, checkmark_bits);
            u8g2_DrawStr(&u8g2, 16, item_y, band_items[i]);
        } else {
            u8g2_DrawStr(&u8g2, 16, item_y, band_items[i]);
        }
        
        if (i == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }
    
    // Footer
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, 64, "Back");
    
    u8g2_DrawXBM(&u8g2, 40, 56, track_next_width, track_next_height, track_next_bits);
    u8g2_DrawStr(&u8g2, 46, 64, "Next");
    
    const char *select_text = "Select";
    int select_text_width = u8g2_GetStrWidth(&u8g2, select_text);
    int select_x = DISPLAY_WIDTH - both_buttons_width - select_text_width - 2;
    u8g2_DrawXBM(&u8g2, select_x, 56, both_buttons_width, both_buttons_height, both_buttons_bits);
    u8g2_DrawStr(&u8g2, select_x + both_buttons_width + 2, 64, select_text);
    
    u8g2_SendBuffer(&u8g2);
}

void lora_band_screen_navigate(menu_direction_t direction) {
    if (direction == MENU_DOWN) {
        selected_item = (selected_item + 1) % band_count;
    } else if (direction == MENU_UP) {
        selected_item = (selected_item - 1 + band_count) % band_count;
    }
}

void lora_band_screen_select(void) {
    lora_config_t config;
    lora_get_config(&config);
    config.frequency = band_frequencies[selected_item];
    lora_set_config(&config);
}
