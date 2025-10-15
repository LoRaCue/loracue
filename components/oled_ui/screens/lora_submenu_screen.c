#include "lora_submenu_screen.h"
#include "lora_driver.h"
#include "oled_ui.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "ui_helpers.h"

extern u8g2_t u8g2;

static int selected_item = 0;
static int scroll_offset = 0;

#define VIEWPORT_SIZE 3

static const char *menu_items[] = {
    "Presets",
    "Frequency",
    "Spr.Factor",
    "Bandwidth",
    "Coding Rate",
    "TX-Power",
    "Band"
};

static const int menu_item_count = sizeof(menu_items) / sizeof(menu_items[0]);

static void get_current_values(char values[][16]) {
    lora_config_t config;
    lora_get_config(&config);
    
    values[0][0] = '\0';  // Presets has no value
    snprintf(values[1], 16, "%.1f MHz", config.frequency / 1000000.0f);
    snprintf(values[2], 16, "SF%d", config.spreading_factor);
    snprintf(values[3], 16, "%d kHz", config.bandwidth);
    snprintf(values[4], 16, "4/%d", config.coding_rate);
    snprintf(values[5], 16, "%d dBm", config.tx_power);
    
    // Determine band from frequency
    if (config.frequency >= 430000000 && config.frequency <= 440000000) {
        snprintf(values[6], 16, "433 MHz");
    } else if (config.frequency >= 863000000 && config.frequency <= 870000000) {
        snprintf(values[6], 16, "868 MHz");
    } else if (config.frequency >= 902000000 && config.frequency <= 928000000) {
        snprintf(values[6], 16, "915 MHz");
    } else {
        snprintf(values[6], 16, "Unknown");
    }
}

void lora_submenu_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Header
    ui_draw_header("LORA SETTINGS");
    
    // Get current values
    char values[7][16];
    get_current_values(values);
    
    // Calculate viewport
    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height = viewport_height / VIEWPORT_SIZE;
    
    // Adjust scroll offset
    if (selected_item < scroll_offset) {
        scroll_offset = selected_item;
    } else if (selected_item >= scroll_offset + VIEWPORT_SIZE) {
        scroll_offset = selected_item - VIEWPORT_SIZE + 1;
    }
    
    // Draw menu items
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    for (int i = 0; i < VIEWPORT_SIZE && (scroll_offset + i) < menu_item_count; i++) {
        int item_idx = scroll_offset + i;
        int item_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + (item_height / 2) + 3;
        
        if (item_idx == selected_item) {
            int bar_y = SEPARATOR_Y_TOP + 2 + (i * item_height) + 1;
            int bar_height = item_height - 2;
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }
        
        u8g2_DrawStr(&u8g2, 4, item_y, menu_items[item_idx]);
        
        // Draw value on right side
        if (values[item_idx][0] != '\0') {
            int value_width = u8g2_GetStrWidth(&u8g2, values[item_idx]);
            u8g2_DrawStr(&u8g2, DISPLAY_WIDTH - value_width - 4, item_y, values[item_idx]);
        }
        
        if (item_idx == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }
    
    // Footer with one-button UI icons
    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);
    
    u8g2_SendBuffer(&u8g2);
}

void lora_submenu_screen_navigate(menu_direction_t direction) {
    if (direction == MENU_DOWN) {
        selected_item = (selected_item + 1) % menu_item_count;
    } else if (direction == MENU_UP) {
        selected_item = (selected_item - 1 + menu_item_count) % menu_item_count;
    }
}

void lora_submenu_screen_select(void) {
    switch (selected_item) {
        case 0: oled_ui_set_screen(OLED_SCREEN_LORA_SETTINGS); break;
        case 1: oled_ui_set_screen(OLED_SCREEN_LORA_FREQUENCY); break;
        case 2: oled_ui_set_screen(OLED_SCREEN_LORA_SF); break;
        case 3: oled_ui_set_screen(OLED_SCREEN_LORA_BW); break;
        case 4: oled_ui_set_screen(OLED_SCREEN_LORA_CR); break;
        case 5: oled_ui_set_screen(OLED_SCREEN_LORA_TXPOWER); break;
        case 6: oled_ui_set_screen(OLED_SCREEN_LORA_BAND); break;
    }
}
