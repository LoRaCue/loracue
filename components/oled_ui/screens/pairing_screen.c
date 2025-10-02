#include "pairing_screen.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "u8g2.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "usb_pairing.h"
#include "device_config.h"

extern u8g2_t u8g2;

static const char* TAG = "pairing_screen";
static bool show_success = false;
static uint64_t success_start_time = 0;
static bool pairing_active = false;

#define SUCCESS_DISPLAY_TIME_US (5 * 1000000) // 5 seconds in microseconds

static void pairing_result_callback(bool success, uint16_t device_id, const char* device_name) {
    if (success) {
        ESP_LOGI(TAG, "Pairing successful with %s (ID: %04X)", device_name, device_id);
        show_success = true;
        success_start_time = esp_timer_get_time();
    } else {
        ESP_LOGE(TAG, "Pairing failed");
        show_success = false;
    }
    pairing_active = false;
}

void pairing_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "USB PAIRING");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
    
    // Check if we should hide success message
    if (show_success && (esp_timer_get_time() - success_start_time) > SUCCESS_DISPLAY_TIME_US) {
        show_success = false;
    }
    
    if (show_success) {
        // Success message
        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawXBM(&u8g2, 4, 32, checkmark_width, checkmark_height, checkmark_bits);
        u8g2_DrawStr(&u8g2, 16, 31, "Pairing");
        u8g2_DrawStr(&u8g2, 16, 43, "successful!");
    } else if (pairing_active) {
        // Show pairing status
        device_config_t config;
        device_config_get(&config);
        
        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        
        if (config.device_mode == DEVICE_MODE_PRESENTER) {
            u8g2_DrawStr(&u8g2, 2, 25, "Sending pairing");
            u8g2_DrawStr(&u8g2, 2, 37, "command to PC...");
            u8g2_DrawStr(&u8g2, 2, 49, "Host mode active");
        } else {
            u8g2_DrawStr(&u8g2, 2, 25, "Waiting for");
            u8g2_DrawStr(&u8g2, 2, 37, "presenter device");
            u8g2_DrawStr(&u8g2, 2, 49, "Device mode ready");
        }
    } else {
        // Instructions
        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawStr(&u8g2, 2, 25, "Connect USB-C cable");
        u8g2_DrawStr(&u8g2, 2, 37, "PC <-> PRESENTER");
        u8g2_DrawStr(&u8g2, 2, 49, "Press OK to start");
    }
    
    // Footer
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, 64, "Back");
    
    u8g2_SendBuffer(&u8g2);
}

void pairing_screen_navigate(menu_direction_t direction) {
    // No navigation needed
}

int pairing_screen_get_selected(void) {
    return 0;
}

void pairing_screen_reset(void) {
    show_success = false;
    success_start_time = 0;
    pairing_active = false;
    usb_pairing_stop();
}

void pairing_screen_select(void) {
    if (!pairing_active && !show_success) {
        ESP_LOGI(TAG, "Starting simple USB pairing");
        pairing_active = true;
        usb_pairing_start(pairing_result_callback);
    }
}
