#include "lora_frequency_screen.h"
#include "lora_driver.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include <stdio.h>

extern u8g2_t u8g2;

static uint32_t frequency_khz = 868000;
static uint32_t min_freq_khz = 863000;
static uint32_t max_freq_khz = 870000;
static bool edit_mode = false;

#define FREQ_STEP_KHZ 200

void lora_frequency_screen_init(void) {
    lora_config_t config;
    lora_get_config(&config);
    frequency_khz = config.frequency / 1000;
    
    // Set limits based on current band
    if (frequency_khz >= 430000 && frequency_khz <= 440000) {
        min_freq_khz = 430000;
        max_freq_khz = 440000;
    } else if (frequency_khz >= 863000 && frequency_khz <= 870000) {
        min_freq_khz = 863000;
        max_freq_khz = 870000;
    } else if (frequency_khz >= 902000 && frequency_khz <= 928000) {
        min_freq_khz = 902000;
        max_freq_khz = 928000;
    }
    
    edit_mode = false;
}

void lora_frequency_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "FREQUENCY");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
    
    // Large centered frequency display
    char freq_str[16];
    snprintf(freq_str, sizeof(freq_str), "%.1f MHz", frequency_khz / 1000.0f);
    
    u8g2_SetFont(&u8g2, u8g2_font_helvB18_tr);
    int text_width = u8g2_GetStrWidth(&u8g2, freq_str);
    int text_x = (DISPLAY_WIDTH - text_width) / 2;
    int text_y = (SEPARATOR_Y_TOP + SEPARATOR_Y_BOTTOM) / 2 + 6;
    u8g2_DrawStr(&u8g2, text_x, text_y, freq_str);
    
    // Footer
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    if (edit_mode) {
        u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
        u8g2_DrawXBM(&u8g2, 8, 56, track_next_width, track_next_height, track_next_bits);
        u8g2_DrawStr(&u8g2, 14, 64, "Up/Down");
        
        int save_text_width = u8g2_GetStrWidth(&u8g2, "Save");
        int save_x = DISPLAY_WIDTH - both_buttons_width - save_text_width - 2;
        u8g2_DrawXBM(&u8g2, save_x, 56, both_buttons_width, both_buttons_height, both_buttons_bits);
        u8g2_DrawStr(&u8g2, save_x + both_buttons_width + 2, 64, "Save");
    } else {
        u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
        u8g2_DrawStr(&u8g2, 8, 64, "Back");
        
        int change_text_width = u8g2_GetStrWidth(&u8g2, "Change");
        int change_x = DISPLAY_WIDTH - both_buttons_width - change_text_width - 2;
        u8g2_DrawXBM(&u8g2, change_x, 56, both_buttons_width, both_buttons_height, both_buttons_bits);
        u8g2_DrawStr(&u8g2, change_x + both_buttons_width + 2, 64, "Change");
    }
    
    u8g2_SendBuffer(&u8g2);
}

void lora_frequency_screen_navigate(menu_direction_t direction) {
    if (!edit_mode) return;
    
    if (direction == MENU_DOWN) {
        if (frequency_khz + FREQ_STEP_KHZ <= max_freq_khz) {
            frequency_khz += FREQ_STEP_KHZ;
        }
    } else if (direction == MENU_UP) {
        if (frequency_khz - FREQ_STEP_KHZ >= min_freq_khz) {
            frequency_khz -= FREQ_STEP_KHZ;
        }
    }
}

void lora_frequency_screen_select(void) {
    if (edit_mode) {
        lora_config_t config;
        lora_get_config(&config);
        config.frequency = frequency_khz * 1000;
        lora_set_config(&config);
        edit_mode = false;
    } else {
        edit_mode = true;
    }
}

bool lora_frequency_screen_is_edit_mode(void) {
    return edit_mode;
}
