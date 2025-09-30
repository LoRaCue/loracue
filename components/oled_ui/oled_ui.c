/**
 * @file oled_ui.c
 * @brief Professional OLED User Interface with u8g2 graphics library
 */

#include "oled_ui.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "version.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include "esp_mac.h"
#include <string.h>

static const char *TAG = "OLED_UI";

static u8g2_t u8g2;
static bool initialized = false;
static oled_screen_t current_screen = OLED_SCREEN_BOOT;
static oled_status_t current_status = {0};

// Menu system state
static int menu_selected_item = 0;
static int submenu_selected_item = 0;

// Menu items
typedef enum {
    MENU_DEVICE_MODE = 0,
    MENU_BATTERY_STATUS,
    MENU_LORA_SETTINGS,
    MENU_CONFIG_MODE,
    MENU_DEVICE_INFO,
    MENU_SYSTEM_INFO,
    MENU_COUNT
} menu_item_t;

static const char* menu_items[] = {
    "Device Mode (PC/STAGE)",
    "Battery Status",
    "LoRa Settings",
    "Configuration Mode", 
    "Device Info",
    "System Info"
};

esp_err_t oled_ui_init(void)
{
    ESP_LOGI(TAG, "Initializing u8g2 OLED UI - PROPER HAL");
    
    // Configure u8g2 HAL for I2C
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.bus.i2c.sda = GPIO_NUM_17;
    u8g2_esp32_hal.bus.i2c.scl = GPIO_NUM_18;
    u8g2_esp32_hal_init(u8g2_esp32_hal);
    
    // Initialize u8g2 with proper HAL
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, 
                                           u8g2_esp32_i2c_byte_cb, 
                                           u8g2_esp32_gpio_and_delay_cb);
    
    // Set I2C address (0x3C shifted left = 0x78)
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);
    
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearDisplay(&u8g2);
    
    initialized = true;
    ESP_LOGI(TAG, "u8g2 OLED UI initialized successfully - PROPER HAL");
    
    // Show boot screen
    oled_ui_set_screen(OLED_SCREEN_BOOT);
    
    return ESP_OK;
}

void oled_ui_handle_button(oled_button_t button, bool long_press)
{
    if (!initialized) return;
    
    switch (current_screen) {
        case OLED_SCREEN_MAIN:
            if (button == OLED_BUTTON_BOTH && long_press) {
                // Enter menu
                current_screen = OLED_SCREEN_MENU;
                menu_selected_item = 0;
                oled_ui_set_screen(OLED_SCREEN_MENU);
            }
            break;
            
        case OLED_SCREEN_MENU:
            if (button == OLED_BUTTON_PREV) {
                // Navigate up
                if (menu_selected_item > 0) {
                    menu_selected_item--;
                    oled_ui_set_screen(OLED_SCREEN_MENU);
                }
            } else if (button == OLED_BUTTON_NEXT) {
                // Navigate down
                if (menu_selected_item < MENU_COUNT - 1) {
                    menu_selected_item++;
                    oled_ui_set_screen(OLED_SCREEN_MENU);
                }
            } else if (button == OLED_BUTTON_BOTH) {
                // Select menu item
                switch (menu_selected_item) {
                    case MENU_DEVICE_MODE:
                        current_screen = OLED_SCREEN_DEVICE_MODE;
                        break;
                    case MENU_BATTERY_STATUS:
                        current_screen = OLED_SCREEN_BATTERY;
                        break;
                    case MENU_LORA_SETTINGS:
                        current_screen = OLED_SCREEN_LORA_SETTINGS;
                        break;
                    case MENU_CONFIG_MODE:
                        current_screen = OLED_SCREEN_CONFIG_MODE;
                        break;
                    case MENU_DEVICE_INFO:
                        current_screen = OLED_SCREEN_DEVICE_INFO;
                        break;
                    case MENU_SYSTEM_INFO:
                        current_screen = OLED_SCREEN_SYSTEM_INFO;
                        break;
                }
                submenu_selected_item = 0;
                oled_ui_set_screen(current_screen);
            }
            break;
            
        default:
            // Back to menu from any submenu
            if (button == OLED_BUTTON_PREV) {
                current_screen = OLED_SCREEN_MENU;
                oled_ui_set_screen(OLED_SCREEN_MENU);
            }
            break;
    }
}

esp_err_t oled_ui_set_screen(oled_screen_t screen)
{
    if (!initialized) {
        ESP_LOGE(TAG, "OLED UI not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Setting screen to: %d", screen);
    current_screen = screen;
    
    u8g2_ClearBuffer(&u8g2);
    
    switch (screen) {
        case OLED_SCREEN_BOOT:
            // Professional boot screen with Helvetica typography
            ESP_LOGI(TAG, "Rendering boot screen");
            
            // Main title "LORACUE" (14pt Helvetica Bold, centered)
            u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
            int title_width = u8g2_GetStrWidth(&u8g2, "LORACUE");
            u8g2_DrawStr(&u8g2, (128 - title_width) / 2, 32, "LORACUE");
            
            // Subtitle "Enterprise Remote" (8pt Helvetica Regular, centered)
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            int subtitle_width = u8g2_GetStrWidth(&u8g2, "Enterprise Remote");
            u8g2_DrawStr(&u8g2, (128 - subtitle_width) / 2, 45, "Enterprise Remote");
            
            // Version and status (8pt Helvetica Regular, corners)
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            u8g2_DrawStr(&u8g2, 0, 60, LORACUE_VERSION_STRING);  // Leftmost side (x=0)
            int init_width = u8g2_GetStrWidth(&u8g2, "Initializing...");
            u8g2_DrawStr(&u8g2, 127 - init_width + 1, 60, "Initializing...");  // Right-aligned to pixel 127
            break;
            
        case OLED_SCREEN_MENU:
            // Main menu with scrollable items
            ESP_LOGI(TAG, "Rendering menu screen");
            
            // Title
            u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
            u8g2_DrawStr(&u8g2, 2, 12, "MENU");
            u8g2_DrawHLine(&u8g2, 0, 15, 128);
            
            // Menu items (show 4 items max, scroll if needed)
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            int start_item = (menu_selected_item > 3) ? menu_selected_item - 3 : 0;
            
            for (int i = 0; i < 4 && (start_item + i) < MENU_COUNT; i++) {
                int item_index = start_item + i;
                int y_pos = 28 + (i * 10);
                
                // Draw selection indicator
                if (item_index == menu_selected_item) {
                    u8g2_DrawStr(&u8g2, 2, y_pos, ">");
                }
                
                // Draw menu item text
                u8g2_DrawStr(&u8g2, 12, y_pos, menu_items[item_index]);
            }
            
            // Navigation hint
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            u8g2_DrawStr(&u8g2, 2, 60, "[<] Back");
            u8g2_DrawStr(&u8g2, 45, 60, "[^v] Navigate");
            u8g2_DrawStr(&u8g2, 100, 60, "[*] OK");
            break;
            
        case OLED_SCREEN_DEVICE_INFO:
            // Device information screen
            ESP_LOGI(TAG, "Rendering device info screen");
            
            // Title
            u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
            u8g2_DrawStr(&u8g2, 2, 12, "DEVICE INFO");
            u8g2_DrawHLine(&u8g2, 0, 15, 128);
            
            // Device information
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            
            // Device name (generate from MAC)
            uint8_t mac[6];
            esp_read_mac(mac, ESP_MAC_WIFI_STA);
            char device_name[16] = "RC-";
            // Convert last 2 MAC bytes to hex
            device_name[3] = "0123456789ABCDEF"[mac[4] >> 4];
            device_name[4] = "0123456789ABCDEF"[mac[4] & 0xF];
            device_name[5] = "0123456789ABCDEF"[mac[5] >> 4];
            device_name[6] = "0123456789ABCDEF"[mac[5] & 0xF];
            device_name[7] = '\0';
            u8g2_DrawStr(&u8g2, 2, 26, "Device: ");
            u8g2_DrawStr(&u8g2, 45, 26, device_name);
            
            // Mode (placeholder - will be from NVS later)
            u8g2_DrawStr(&u8g2, 2, 36, "Mode: STAGE Remote");
            
            // LoRa channel (placeholder)
            u8g2_DrawStr(&u8g2, 2, 46, "LoRa: 868.1 MHz");
            
            // Device ID (from MAC)
            char device_id[16] = "ID: ";
            device_id[4] = "0123456789ABCDEF"[mac[4] >> 4];
            device_id[5] = "0123456789ABCDEF"[mac[4] & 0xF];
            device_id[6] = "0123456789ABCDEF"[mac[5] >> 4];
            device_id[7] = "0123456789ABCDEF"[mac[5] & 0xF];
            device_id[8] = '\0';
            u8g2_DrawStr(&u8g2, 2, 56, device_id);
            
            // Navigation hint
            u8g2_DrawStr(&u8g2, 2, 62, "[<] Back");
            break;
            
        case OLED_SCREEN_DEVICE_MODE:
        case OLED_SCREEN_BATTERY:
        case OLED_SCREEN_LORA_SETTINGS:
        case OLED_SCREEN_CONFIG_MODE:
        case OLED_SCREEN_CONFIG_ACTIVE:
            // Placeholder screens - to be implemented
            u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
            u8g2_DrawStr(&u8g2, 2, 20, "Coming Soon");
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            u8g2_DrawStr(&u8g2, 2, 60, "[<] Back");
            break;
        case OLED_SCREEN_SYSTEM_INFO:
            // System information screen
            ESP_LOGI(TAG, "Rendering system info screen");
            
            // Title
            u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
            u8g2_DrawStr(&u8g2, 2, 12, "SYSTEM INFO");
            u8g2_DrawHLine(&u8g2, 0, 15, 128);
            
            // System information
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            
            // Firmware version
            u8g2_DrawStr(&u8g2, 2, 26, "Firmware: ");
            u8g2_DrawStr(&u8g2, 55, 26, LORACUE_VERSION_STRING);
            
            // Hardware
            u8g2_DrawStr(&u8g2, 2, 36, "Hardware: Heltec LoRa V3");
            
            // ESP-IDF version
            u8g2_DrawStr(&u8g2, 2, 46, "ESP-IDF: ");
            u8g2_DrawStr(&u8g2, 50, 46, IDF_VER);
            
            // Free heap memory - simple conversion without snprintf
            uint32_t heap_kb = esp_get_free_heap_size() / 1024;
            char heap_str[20] = "Free RAM: ";
            char *p = heap_str + 10; // Point after "Free RAM: "
            
            // Simple integer to string conversion
            if (heap_kb >= 100) {
                *p++ = '0' + (heap_kb / 100);
                heap_kb %= 100;
            }
            if (heap_kb >= 10 || p > heap_str + 10) {
                *p++ = '0' + (heap_kb / 10);
                heap_kb %= 10;
            }
            *p++ = '0' + heap_kb;
            *p++ = 'K';
            *p++ = 'B';
            *p = '\0';
            
            u8g2_DrawStr(&u8g2, 2, 56, heap_str);
            
            // Navigation hint
            u8g2_DrawStr(&u8g2, 2, 62, "[<] Back");
            break;
        case OLED_SCREEN_LOW_BATTERY:
        case OLED_SCREEN_CONNECTION_LOST:
            // Placeholder screens - to be implemented
            u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
            u8g2_DrawStr(&u8g2, 2, 20, "Coming Soon");
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            u8g2_DrawStr(&u8g2, 2, 60, "[<] Back");
            break;
            
        case OLED_SCREEN_MAIN:
            // Professional main screen with status bar layout
            ESP_LOGI(TAG, "Rendering main screen");
            
            // Top status bar: LORACUE moved 1px more left
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            u8g2_DrawStr(&u8g2, -1, 8, "LORACUE");  // Start at x=-1 (1px more left)
            
            // Status icons (positioned on right side)
            // USB-C icon (closed outline with 1px rounded corners) - moved 1px left
            u8g2_DrawHLine(&u8g2, 94, 2, 8);     // Top edge (1px gap each side)
            u8g2_DrawHLine(&u8g2, 94, 7, 8);     // Bottom edge (1px gap each side)
            u8g2_DrawVLine(&u8g2, 93, 3, 3);     // Left edge (1px gap each side)
            u8g2_DrawVLine(&u8g2, 103, 3, 3);    // Right edge (1px gap each side)
            u8g2_DrawHLine(&u8g2, 95, 4, 6);     // Contact strip
            
            // RF/LoRa icon (signal bars) - moved 1px left, last bar full 6px height
            u8g2_DrawVLine(&u8g2, 107, 6, 2);    // Signal bar 1 (short)
            u8g2_DrawVLine(&u8g2, 109, 5, 3);    // Signal bar 2 (medium)
            u8g2_DrawVLine(&u8g2, 111, 4, 4);    // Signal bar 3 (tall)
            u8g2_DrawVLine(&u8g2, 113, 2, 6);    // Signal bar 4 (full 6px height)
            
            // Battery icon (moved 1px left) - filled left to right
            u8g2_DrawBox(&u8g2, 117, 2, 2, 6);   // Battery segment 1 (filled) - 75% = first 3 filled
            u8g2_DrawBox(&u8g2, 120, 2, 2, 6);   // Battery segment 2 (filled)
            u8g2_DrawBox(&u8g2, 123, 2, 2, 6);   // Battery segment 3 (filled) 
            u8g2_DrawFrame(&u8g2, 126, 2, 2, 6); // Battery segment 4 (empty, ends at 127)
            
            // Separator line at 9px
            u8g2_DrawHLine(&u8g2, 0, 9, 128);
            
            // Main content area - "PRESENTER" (same font as init screen)
            u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
            int ready_width = u8g2_GetStrWidth(&u8g2, "PRESENTER");
            u8g2_DrawStr(&u8g2, (128 - ready_width) / 2, 30, "PRESENTER");
            
            // Button hints - symmetrically spaced
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            u8g2_DrawStr(&u8g2, 5, 43, "[<PREV]");
            int next_width = u8g2_GetStrWidth(&u8g2, "[NEXT>]");
            u8g2_DrawStr(&u8g2, 128 - next_width - 5, 43, "[NEXT>]");  // Same 5px margin from right
            
            // Bottom separator moved to Y=53
            u8g2_DrawHLine(&u8g2, 0, 53, 128);
            
            // Bottom bar: Device name moved 1px more left, Menu hint at rightmost edge
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            u8g2_DrawStr(&u8g2, -1, 63, "RC-A4B2");  // Bottom line (Y=63)
            int menu_width = u8g2_GetStrWidth(&u8g2, "3s [<+>] Menu");
            u8g2_DrawStr(&u8g2, 127 - menu_width + 1, 63, "3s [<+>] Menu");  // Bottom line (Y=63)
            break;
    }
    
    ESP_LOGI(TAG, "Sending buffer to display");
    u8g2_SendBuffer(&u8g2);
    ESP_LOGI(TAG, "Buffer sent successfully");
    return ESP_OK;
}

esp_err_t oled_ui_update_status(const oled_status_t *status)
{
    if (!initialized || !status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Only update if status changed to prevent unnecessary refreshes
    if (memcmp(&current_status, status, sizeof(oled_status_t)) != 0) {
        current_status = *status;
        
        // Refresh current screen if it's the main screen
        if (current_screen == OLED_SCREEN_MAIN) {
            return oled_ui_set_screen(OLED_SCREEN_MAIN);
        }
    }
    
    return ESP_OK;
}

esp_err_t oled_ui_show_message(const char *title, const char *message, uint32_t timeout_ms)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    u8g2_ClearBuffer(&u8g2);
    
    if (title) {
        u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
        u8g2_DrawStr(&u8g2, 0, 15, title);
    }
    
    if (message) {
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
        u8g2_DrawStr(&u8g2, 0, 35, message);
    }
    
    u8g2_SendBuffer(&u8g2);
    
    if (timeout_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(timeout_ms));
        oled_ui_set_screen(current_screen);
    }
    
    return ESP_OK;
}

esp_err_t oled_ui_clear(void)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
    return ESP_OK;
}

u8g2_t* oled_ui_get_u8g2(void)
{
    return initialized ? &u8g2 : NULL;
}
