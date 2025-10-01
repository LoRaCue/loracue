#include "menu_screen.h"
#include "ui_config.h"
#include "ui_icons.h"
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
static int scroll_offset = 0;

#define MAX_VISIBLE_ITEMS 5  // Items fit exactly: 12+10*4=52, bottom bar at 62

void menu_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Menu items (no title bar for space)
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    // Calculate visible range
    int visible_start = scroll_offset;
    int visible_end = scroll_offset + MAX_VISIBLE_ITEMS;
    if (visible_end > menu_item_count) visible_end = menu_item_count;
    
    for (int i = visible_start; i < visible_end; i++) {
        int display_index = i - visible_start;
        int y = 12 + (display_index * 10);
        
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
    
    // Scroll indicators (custom bitmaps)
    if (scroll_offset > 0) {
        u8g2_DrawXBM(&u8g2, 118, 5, scroll_up_width, scroll_up_height, scroll_up_bits);
    }
    if (visible_end < menu_item_count) {
        u8g2_DrawXBM(&u8g2, 118, 45, scroll_down_width, scroll_down_height, scroll_down_bits);
    }
    
    // Navigation hints with icons (compact bottom bar)
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawXBM(&u8g2, 2, 56, updown_nav_width, updown_nav_height, updown_nav_bits);
    u8g2_DrawStr(&u8g2, 17, 62, "Up/Down");
    
    // Move Select to rightmost position
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);  // Explicitly set font again
    const char* select_text = "Select";
    int select_text_width = u8g2_GetStrWidth(&u8g2, select_text);
    int select_x = DISPLAY_WIDTH - both_buttons_width - select_text_width - 2;
    u8g2_DrawXBM(&u8g2, select_x, 56, both_buttons_width, both_buttons_height, both_buttons_bits);
    u8g2_DrawStr(&u8g2, select_x + both_buttons_width + 2, 62, select_text);
    
    u8g2_SendBuffer(&u8g2);
}

void menu_screen_navigate(menu_direction_t direction) {
    switch (direction) {
        case MENU_UP:
            selected_item = (selected_item - 1 + menu_item_count) % menu_item_count;
            // Handle wrap-around from top to bottom
            if (selected_item == menu_item_count - 1) {
                // Wrapped to last item - scroll to show it
                scroll_offset = menu_item_count - MAX_VISIBLE_ITEMS;
                if (scroll_offset < 0) scroll_offset = 0;
            } else if (selected_item < scroll_offset) {
                // Normal scroll up
                scroll_offset = selected_item;
            }
            break;
        case MENU_DOWN:
            selected_item = (selected_item + 1) % menu_item_count;
            // Handle wrap-around from bottom to top
            if (selected_item == 0) {
                // Wrapped to first item - scroll to top
                scroll_offset = 0;
            } else if (selected_item >= scroll_offset + MAX_VISIBLE_ITEMS) {
                // Normal scroll down
                scroll_offset = selected_item - MAX_VISIBLE_ITEMS + 1;
            }
            break;
    }
}

int menu_screen_get_selected(void) {
    return selected_item;
}

void menu_screen_reset(void) {
    selected_item = 0;
    scroll_offset = 0;
}
