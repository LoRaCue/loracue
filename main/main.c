#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "bsp.h"
#include "lora_driver.h"
#include "display_ui.h"

static const char *TAG = "LORACUE";

#define FIRMWARE_VERSION "1.0.0-dev"

void app_main(void)
{
    ESP_LOGI(TAG, "LoRaCue starting up - Version %s", FIRMWARE_VERSION);
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize Board Support Package
    ESP_ERROR_CHECK(bsp_init());
    
    // Initialize hardware components
    const bsp_interface_t* bsp = bsp_get_interface();
    
    ESP_LOGI(TAG, "Initializing LoRa module");
    ESP_ERROR_CHECK(bsp->lora_init());
    ESP_ERROR_CHECK(lora_driver_init());
    
    ESP_LOGI(TAG, "Initializing display");
    ESP_ERROR_CHECK(bsp->display_init());
    ESP_ERROR_CHECK(display_ui_init());
    
    ESP_LOGI(TAG, "Initializing buttons");
    ESP_ERROR_CHECK(bsp->buttons_init());
    
    // Show boot logo
    display_show_boot_logo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Read and display battery voltage
    float battery_voltage = bsp->read_battery();
    ESP_LOGI(TAG, "Battery voltage: %.2fV", battery_voltage);
    
    // Show main screen
    display_show_main_screen("STAGE-DEMO", battery_voltage, 85); // 85% signal strength
    
    ESP_LOGI(TAG, "LoRaCue initialization complete");
    
    // Main application loop
    while (1) {
        // Simple hello world - just blink the display
        vTaskDelay(pdMS_TO_TICKS(5000));
        
        // Update battery reading
        battery_voltage = bsp->read_battery();
        display_show_main_screen("STAGE-DEMO", battery_voltage, 85);
        
        ESP_LOGI(TAG, "Hello World! Battery: %.2fV", battery_voltage);
    }
}
