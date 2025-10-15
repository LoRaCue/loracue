#include "lora_band_screen.h"
#include "lora_driver.h"
#include "lora_bands.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "ui_helpers.h"
#include <stdio.h>
#include <string.h>

extern u8g2_t u8g2;

static int selected_item = 0;

void lora_band_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Header
    ui_draw_header("BAND");
    
    // Get current config
    lora_config_t config;
    lora_get_config(&config);
    
    int band_count = lora_bands_get_count();
    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height = viewport_height / band_count;
    
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    for (int i = 0; i < band_count; i++) {
        const lora_band_profile_t *profile = lora_bands_get_profile(i);
        if (!profile) continue;
        
        int item_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + (item_height / 2) + 3;
        
        if (i == selected_item) {
            int bar_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + 1;
            int bar_height = item_height - 2;
            if (i == band_count - 1) bar_height -= 1;
            
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }
        
        // Show checkmark for current band
        bool is_current = (strcmp(config.band_id, profile->id) == 0);
        
        // Display band name (e.g., "433 MHz")
        char band_label[32];
        snprintf(band_label, sizeof(band_label), "%lu MHz", (unsigned long)(profile->optimal_center_khz / 1000));
        
        if (is_current) {
            u8g2_DrawXBM(&u8g2, 2, item_y - 6, checkmark_width, checkmark_height, checkmark_bits);
            u8g2_DrawStr(&u8g2, 16, item_y, band_label);
        } else {
            u8g2_DrawStr(&u8g2, 16, item_y, band_label);
        }
        
        if (i == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }
    
    // Footer with one-button UI icons
    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);
    
    u8g2_SendBuffer(&u8g2);
}

void lora_band_screen_navigate(menu_direction_t direction) {
    int band_count = lora_bands_get_count();
    if (direction == MENU_DOWN) {
        selected_item = (selected_item + 1) % band_count;
    } else if (direction == MENU_UP) {
        selected_item = (selected_item - 1 + band_count) % band_count;
    }
}

void lora_band_screen_select(void) {
    const lora_band_profile_t *profile = lora_bands_get_profile(selected_item);
    if (!profile) return;
    
    lora_config_t config;
    lora_get_config(&config);
    
    // Set band ID and center frequency
    strncpy(config.band_id, profile->id, sizeof(config.band_id) - 1);
    config.frequency = profile->optimal_center_khz * 1000; // Convert kHz to Hz
    
    lora_set_config(&config);
}
