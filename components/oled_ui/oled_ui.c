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
#include "esp_mac.h"
#include <string.h>

static const char* TAG = "oled_ui";

// Global u8g2 instance (initialized by BSP)
extern u8g2_t u8g2;

esp_err_t oled_ui_init(void) {
    ESP_LOGI(TAG, "Initializing OLED UI");
    
    // u8g2 is already initialized by BSP
    // Just verify it's ready
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

oled_screen_t oled_ui_get_screen(void) {
    return ui_screen_controller_get_current();
}

esp_err_t oled_ui_show_message(const char *title, const char *message, uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Message: %s - %s", title, message);
    // TODO: Implement message overlay screen
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
