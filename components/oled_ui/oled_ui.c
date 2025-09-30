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
            
            // Main title "LoRaCue" (14pt Helvetica Bold, centered)
            u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
            int title_width = u8g2_GetStrWidth(&u8g2, "LoRaCue");
            u8g2_DrawStr(&u8g2, (128 - title_width) / 2, 32, "LoRaCue");
            
            // Subtitle "Enterprise Remote" (8pt Helvetica Regular, centered)
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            int subtitle_width = u8g2_GetStrWidth(&u8g2, "Enterprise Remote");
            u8g2_DrawStr(&u8g2, (128 - subtitle_width) / 2, 45, "Enterprise Remote");
            
            // Version and status (6pt Helvetica Regular, corners)
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            u8g2_DrawStr(&u8g2, 2, 60, LORACUE_VERSION_STRING);
            u8g2_DrawStr(&u8g2, 85, 60, "Initializing...");
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
            
        case OLED_SCREEN_DEVICE_MODE:
        case OLED_SCREEN_BATTERY:
        case OLED_SCREEN_LORA_SETTINGS:
        case OLED_SCREEN_CONFIG_MODE:
        case OLED_SCREEN_CONFIG_ACTIVE:
        case OLED_SCREEN_DEVICE_INFO:
        case OLED_SCREEN_SYSTEM_INFO:
        case OLED_SCREEN_LOW_BATTERY:
        case OLED_SCREEN_CONNECTION_LOST:
            // Placeholder screens - to be implemented
            u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
            u8g2_DrawStr(&u8g2, 2, 20, "Coming Soon");
            u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
            u8g2_DrawStr(&u8g2, 2, 60, "[<] Back");
            break;
            
        case OLED_SCREEN_MAIN:
            // Main status screen
            u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
            u8g2_DrawStr(&u8g2, 0, 10, "LoRaCue Ready");
            
            // Battery indicator
            char battery_str[16];
            snprintf(battery_str, sizeof(battery_str), "Bat: %d%%", current_status.battery_level);
            u8g2_DrawStr(&u8g2, 0, 25, battery_str);
            
            // LoRa status
            u8g2_DrawStr(&u8g2, 0, 40, current_status.lora_connected ? "LoRa: OK" : "LoRa: --");
            
            // USB status
            u8g2_DrawStr(&u8g2, 70, 40, current_status.usb_connected ? "USB: OK" : "USB: --");
            
            // Device info
            char device_str[32];
            snprintf(device_str, sizeof(device_str), "%.8s (%04X)", 
                    current_status.device_name, current_status.device_id);
            u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
            u8g2_DrawStr(&u8g2, 0, 55, device_str);
            break;
            
        case OLED_SCREEN_MENU:
            u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
            u8g2_DrawStr(&u8g2, 0, 12, "Menu");
            u8g2_DrawStr(&u8g2, 0, 28, "1. Device Info");
            u8g2_DrawStr(&u8g2, 0, 42, "2. Settings");
            u8g2_DrawStr(&u8g2, 0, 56, "3. Pair Device");
            break;
            
        case OLED_SCREEN_PAIRING:
            u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
            u8g2_DrawStr(&u8g2, 0, 15, "Device Pairing");
            u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
            u8g2_DrawStr(&u8g2, 0, 35, "Connect USB cable");
            u8g2_DrawStr(&u8g2, 0, 50, "Waiting for host...");
            break;
            
        case OLED_SCREEN_ERROR:
            u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
            u8g2_DrawStr(&u8g2, 0, 15, "System Error");
            u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
            u8g2_DrawStr(&u8g2, 0, 35, "Check hardware");
            u8g2_DrawStr(&u8g2, 0, 50, "connections");
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
