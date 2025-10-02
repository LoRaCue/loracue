#include "pairing_screen.h"
#include "ui_config.h"
#include "ui_icons.h"
#include "u8g2.h"
#include "esp_log.h"
#include "esp_timer.h"

extern u8g2_t u8g2;

static const char* TAG = "pairing_screen";
static bool show_success = false;
static uint64_t success_start_time = 0;

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
}

void pairing_screen_draw(void) {
    u8g2_ClearBuffer(&u8g2);
    
    // Header
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    u8g2_DrawStr(&u8g2, 2, 8, "DEVICE PAIRING");
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_TOP, DISPLAY_WIDTH);
    
    // Check if we should hide success message
    if (show_success && (esp_timer_get_time() - success_start_time) > SUCCESS_DISPLAY_TIME_US) {
        show_success = false;
    }
    
    if (show_success) {
        // Success message with checkmark (centered vertically)
        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawXBM(&u8g2, 4, 32, checkmark_width, checkmark_height, checkmark_bits);
        u8g2_DrawStr(&u8g2, 16, 31, "Pairing");
        u8g2_DrawStr(&u8g2, 16, 43, "successful!");
    } else {
        // Normal pairing instructions
        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawStr(&u8g2, 2, 25, "Pair modules");
        u8g2_DrawStr(&u8g2, 2, 37, "PC <-> PRESENTER");
        
        // Centered USB-C
        const char* usb_text = "USB-C";
        int usb_width = u8g2_GetStrWidth(&u8g2, usb_text);
        u8g2_DrawStr(&u8g2, (DISPLAY_WIDTH - usb_width) / 2, 49, usb_text);
    }
    
    // Footer with navigation
    u8g2_DrawHLine(&u8g2, 0, SEPARATOR_Y_BOTTOM, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    // Left: Back arrow
    u8g2_DrawXBM(&u8g2, 2, 56, arrow_prev_width, arrow_prev_height, arrow_prev_bits);
    u8g2_DrawStr(&u8g2, 8, 64, "Back");
    
    u8g2_SendBuffer(&u8g2);
}

void pairing_screen_navigate(menu_direction_t direction) {
    // No navigation needed for simple pairing screen
}

int pairing_screen_get_selected(void) {
    return 0;
}

void pairing_screen_reset(void) {
    show_success = false;
    success_start_time = 0;
    // Start pairing automatically when entering screen
    ESP_LOGI(TAG, "Starting USB-C pairing process");
    // TODO: Implement LoRa pairing instead of USB pairing
}

void pairing_screen_select(void) {
    // No manual pairing trigger needed - starts automatically
}
