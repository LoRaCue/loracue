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
            // Boot screen with logo and version
            ESP_LOGI(TAG, "Rendering boot screen");
            
            u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
            u8g2_DrawStr(&u8g2, 10, 25, "LoRaCue");
            
            u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
            u8g2_DrawStr(&u8g2, 15, 40, LORACUE_VERSION_STRING);
            u8g2_DrawStr(&u8g2, 20, 55, "Initializing...");
            
            // Draw a frame around the screen
            u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
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
