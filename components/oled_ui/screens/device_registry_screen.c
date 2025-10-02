#include "device_registry_screen.h"
#include "device_registry.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "u8g2.h"
#include "esp_timer.h"

extern u8g2_t u8g2;

static int selected_device = 0;
static int scroll_offset = 0;
static paired_device_t devices[MAX_PAIRED_DEVICES];
static size_t device_count = 0;

#define MAX_VISIBLE_DEVICES 4

static void refresh_device_list(void) {
    device_registry_list(devices, MAX_PAIRED_DEVICES, &device_count);
}

void device_registry_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Refresh device list
    refresh_device_list();
    
    // Title
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    u8g2_DrawStr(&u8g2, 2, 10, "Device Registry");
    
    if (device_count == 0) {
        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawStr(&u8g2, 2, 30, "No devices paired");
        u8g2_DrawStr(&u8g2, 2, 42, "Use config mode to");
        u8g2_DrawStr(&u8g2, 2, 52, "pair new devices");
    } else {
        // Device list
        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        
        int visible_start = scroll_offset;
        int visible_end = scroll_offset + MAX_VISIBLE_DEVICES;
        if (visible_end > device_count) visible_end = device_count;
        
        for (int i = visible_start; i < visible_end; i++) {
            int display_index = i - visible_start;
            int y = 22 + (display_index * 10);
            
            if (i == selected_device) {
                // Highlight selected device
                u8g2_DrawBox(&u8g2, 0, y - 8, DISPLAY_WIDTH, 9);
                u8g2_SetDrawColor(&u8g2, 0);
                u8g2_DrawStr(&u8g2, 4, y, devices[i].device_name);
                u8g2_SetDrawColor(&u8g2, 1);
            } else {
                u8g2_DrawStr(&u8g2, 4, y, devices[i].device_name);
            }
            
            // Status indicator (online if seen recently)
            uint32_t current_time = esp_timer_get_time() / 1000000; // Convert to seconds
            bool is_online = (current_time - devices[i].last_seen) < 300; // 5 minutes
            
            if (is_online) {
                u8g2_DrawCircle(&u8g2, 118, y - 3, 2, U8G2_DRAW_ALL);
            } else {
                u8g2_DrawCircle(&u8g2, 118, y - 3, 2, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
            }
        }
        
        // Scroll indicators
        if (scroll_offset > 0) {
            int selected_y = 22 + ((selected_device - scroll_offset) * 10);
            int lightbar_top = selected_y - 8;
            int lightbar_bottom = selected_y + 1;
            
            if (15 >= lightbar_top && 15 + scroll_up_height <= lightbar_bottom) {
                u8g2_SetDrawColor(&u8g2, 0);
            }
            u8g2_DrawXBM(&u8g2, 119, 15, scroll_up_width, scroll_up_height, scroll_up_bits);
            u8g2_SetDrawColor(&u8g2, 1);
        }
        if (visible_end < device_count) {
            int selected_y = 22 + ((selected_device - scroll_offset) * 10);
            int lightbar_top = selected_y - 8;
            int lightbar_bottom = selected_y + 1;
            
            if (45 >= lightbar_top && 45 + scroll_down_height <= lightbar_bottom) {
                u8g2_SetDrawColor(&u8g2, 0);
            }
            u8g2_DrawXBM(&u8g2, 119, 45, scroll_down_width, scroll_down_height, scroll_down_bits);
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }
    
    // Bottom navigation
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, 64, "Back");
    
    if (device_count > 0) {
        const char* action_text = "Remove";
        int action_text_width = u8g2_GetStrWidth(&u8g2, action_text);
        int action_x = DISPLAY_WIDTH - both_buttons_width - action_text_width - 2;
        u8g2_DrawXBM(&u8g2, action_x, 56, both_buttons_width, both_buttons_height, both_buttons_bits);
        u8g2_DrawStr(&u8g2, action_x + both_buttons_width + 2, 64, action_text);
    }
    
    u8g2_SendBuffer(&u8g2);
}

void device_registry_screen_navigate(menu_direction_t direction) {
    if (device_count == 0) return;
    
    switch (direction) {
        case MENU_UP:
            selected_device = (selected_device - 1 + device_count) % device_count;
            if (selected_device == device_count - 1) {
                scroll_offset = device_count - MAX_VISIBLE_DEVICES;
                if (scroll_offset < 0) scroll_offset = 0;
            } else if (selected_device < scroll_offset) {
                scroll_offset = selected_device;
            }
            break;
        case MENU_DOWN:
            selected_device = (selected_device + 1) % device_count;
            if (selected_device == 0) {
                scroll_offset = 0;
            } else if (selected_device >= scroll_offset + MAX_VISIBLE_DEVICES) {
                scroll_offset = selected_device - MAX_VISIBLE_DEVICES + 1;
            }
            break;
    }
}

int device_registry_screen_get_selected(void) {
    return selected_device;
}

void device_registry_screen_reset(void) {
    selected_device = 0;
    scroll_offset = 0;
    refresh_device_list();
}
