/**
 * @file main.c
 * @brief I2Console ESP-IDF Example
 * 
 * Demonstrates automatic I2Console detection and ESP_LOG mirroring.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "i2console.h"

static const char *TAG = "example";

#define I2C_MASTER_SCL_IO    22    // GPIO for I2C SCL
#define I2C_MASTER_SDA_IO    21    // GPIO for I2C SDA
#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   100000

/**
 * @brief Initialize I2C master
 */
static void i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
    
    ESP_LOGI(TAG, "I2C master initialized on SDA=%d SCL=%d", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
}

void app_main(void)
{
    ESP_LOGI(TAG, "I2Console Example Starting");
    
    // Initialize I2C
    i2c_master_init();
    
    // Initialize I2Console (auto-detects device)
    esp_err_t ret = i2console_init(I2C_MASTER_NUM, I2CONSOLE_DEFAULT_ADDR);
    
    if (ret == ESP_OK) {
        // Get firmware version
        char version[16];
        if (i2console_get_version(version) == ESP_OK) {
            ESP_LOGI(TAG, "I2Console firmware: %s", version);
        }
        
        ESP_LOGI(TAG, "All ESP_LOG output is now mirrored to I2Console!");
    } else {
        ESP_LOGW(TAG, "I2Console not found - continuing with UART only");
    }
    
    // Demo: Generate various log levels
    int counter = 0;
    while (1) {
        ESP_LOGI(TAG, "Counter: %d", counter);
        ESP_LOGD(TAG, "Debug message %d", counter);
        
        if (counter % 10 == 0) {
            ESP_LOGW(TAG, "Warning at count %d", counter);
        }
        
        if (counter % 20 == 0) {
            ESP_LOGE(TAG, "Error simulation at count %d", counter);
        }
        
        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
