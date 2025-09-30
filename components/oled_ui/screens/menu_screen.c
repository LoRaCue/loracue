#include "menu_screen.h"
#include "ui_config.h"
#include "u8g2.h"

extern u8g2_t u8g2;

static const char* menu_items[] = {
    "Device Mode",
    "Battery Status", 
    "LoRa Settings",
    "Configuration Mode",
    "Device Info",
    "System Info"
};

static const int menu_item_count = sizeof(menu_items) / sizeof(menu_items[0]);
static int selected_item = 0;

void menu_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Title
    u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
    u8g2_DrawStr(&u8g2, 2, 12, "MENU");
    u8g2_DrawHLine(&u8g2, 0, 15, DISPLAY_WIDTH);
    
    // Menu items
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    for (int i = 0; i < menu_item_count; i++) {
        int y = 28 + (i * 10);
        if (y > 55) break;  // Don't draw items that won't fit
        
        if (i == selected_item) {
            // Highlight selected item
            u8g2_DrawBox(&u8g2, 0, y - 8, DISPLAY_WIDTH, 9);
            u8g2_SetDrawColor(&u8g2, 0);  // Invert color for text
            u8g2_DrawStr(&u8g2, 4, y, menu_items[i]);
            u8g2_SetDrawColor(&u8g2, 1);  // Reset color
        } else {
            u8g2_DrawStr(&u8g2, 4, y, menu_items[i]);
        }
    }
    
    // Navigation hints
    u8g2_DrawStr(&u8g2, 2, 62, "[<] Up  [>] Down  [<+>] Select");
    
    u8g2_SendBuffer(&u8g2);
}

void menu_screen_navigate(menu_direction_t direction) {
    switch (direction) {
        case MENU_UP:
            selected_item = (selected_item - 1 + menu_item_count) % menu_item_count;
            break;
        case MENU_DOWN:
            selected_item = (selected_item + 1) % menu_item_count;
            break;
    }
}

int menu_screen_get_selected(void) {
    return selected_item;
}

void menu_screen_reset(void) {
    selected_item = 0;
}
