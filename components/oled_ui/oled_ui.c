#include "oled_ui.h"
#include "ui_screen_controller.h"
#include "ui_data_provider.h"
#include "ui_monitor_task.h"
#include "bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "version.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include "esp_mac.h"
#include <string.h>

static const char* TAG = "oled_ui";

// Global u8g2 instance
u8g2_t u8g2;

esp_err_t oled_ui_init(void) {
    ESP_LOGI(TAG, "Initializing OLED UI");
    
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
    
    // Initialize screen controller
    ui_screen_controller_init();
    
    ESP_LOGI(TAG, "OLED UI initialized successfully");
    return ESP_OK;
}

esp_err_t oled_ui_set_screen(oled_screen_t screen) {
    ui_screen_controller_set(screen, NULL);
    return ESP_OK;
}

void oled_ui_handle_button(oled_button_t button, bool long_press) {
    ui_screen_controller_handle_button(button, long_press);
}

esp_err_t oled_ui_update_status(const oled_status_t *status) {
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Update data provider with new values
    esp_err_t ret = ui_data_provider_force_update(
        status->usb_connected, 
        status->lora_connected, 
        status->battery_level
    );
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to update status: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Update screen controller
    ui_screen_controller_update(NULL);
    return ESP_OK;
}
