#include "lora_slot_screen.h"
#include "device_config.h"
#include "oled_ui.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "ui_helpers.h"

extern u8g2_t u8g2;

static int selected_slot = 0;
static int scroll_offset = 0;

#define VIEWPORT_SIZE 4
#define SLOT_COUNT 16

void lora_slot_screen_init(void) {
    device_config_t config;
    device_config_get(&config);
    selected_slot = config.slot_id - 1; // Convert 1-16 to 0-15
    scroll_offset = 0;
}

void lora_slot_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    device_config_t config;
    device_config_get(&config);
    
    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "SLOT SELECTION");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
    
    // Calculate viewport
    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height = viewport_height / VIEWPORT_SIZE;
    
    // Adjust scroll offset
    if (selected_slot < scroll_offset) {
        scroll_offset = selected_slot;
    } else if (selected_slot >= scroll_offset + VIEWPORT_SIZE) {
        scroll_offset = selected_slot - VIEWPORT_SIZE + 1;
    }
    
    // Draw slot items
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    for (int i = 0; i < VIEWPORT_SIZE && (scroll_offset + i) < SLOT_COUNT; i++) {
        int slot_idx = scroll_offset + i;
        int slot_num = slot_idx + 1; // Display 1-16
        int item_y = SEPARATOR_Y_TOP + (i * item_height) + (item_height / 2) + 3;
        
        if (slot_idx == selected_slot) {
            int bar_y = SEPARATOR_Y_TOP + (i * item_height) + 1;
            int bar_height = item_height - 2;
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }
        
        char slot_text[20];
        snprintf(slot_text, sizeof(slot_text), "Slot %d", slot_num);
        u8g2_DrawStr(&u8g2, 4, item_y, slot_text);
        
        // Draw checkmark if current slot
        if (slot_num == config.slot_id) {
            u8g2_DrawStr(&u8g2, DISPLAY_WIDTH - 12, item_y, "\x2713"); // Checkmark
        }
        
        if (slot_idx == selected_slot) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }
    
    // Footer with one-button UI icons
    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);
    
    u8g2_SendBuffer(&u8g2);
}

void lora_slot_screen_navigate(menu_direction_t direction) {
    if (direction == MENU_DOWN) {
        selected_slot = (selected_slot + 1) % SLOT_COUNT;
    } else if (direction == MENU_UP) {
        selected_slot = (selected_slot - 1 + SLOT_COUNT) % SLOT_COUNT;
    }
}

void lora_slot_screen_select(void) {
    device_config_t config;
    device_config_get(&config);
    config.slot_id = selected_slot + 1; // Convert 0-15 to 1-16
    device_config_set(&config);
    oled_ui_set_screen(OLED_SCREEN_LORA_SUBMENU);
}
