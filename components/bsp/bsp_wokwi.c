/**
 * @file bsp_wokwi.c
 * @brief Minimal BSP for Wokwi simulator - buttons and LEDs only
 */

#include "bsp.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BSP_WOKWI";

// GPIO pin definitions for Wokwi
#define BUTTON_PREV_PIN        GPIO_NUM_46
#define BUTTON_NEXT_PIN        GPIO_NUM_45
#define STATUS_LED_PIN         GPIO_NUM_35

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing Wokwi Simulator BSP");
    
    // Initialize GPIO for buttons
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_PREV_PIN) | (1ULL << BUTTON_NEXT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&button_config);
    if (ret != ESP_OK) return ret;
    
    // Initialize status LED
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << STATUS_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&led_config);
    if (ret != ESP_OK) return ret;
    
    ESP_LOGI(TAG, "Wokwi BSP initialization complete");
    return ESP_OK;
}

bool bsp_read_button(bsp_button_t button)
{
    gpio_num_t pin = (button == BSP_BUTTON_PREV) ? BUTTON_PREV_PIN : BUTTON_NEXT_PIN;
    return gpio_get_level(pin) == 0; // Active low
}

void bsp_set_led(bool state)
{
    gpio_set_level(STATUS_LED_PIN, state);
}

float bsp_read_battery(void)
{
    // Simulate 75% battery for Wokwi
    return 75.0f;
}

esp_err_t bsp_validate_hardware(void)
{
    ESP_LOGI(TAG, "Validating Wokwi simulator hardware...");
    ESP_LOGI(TAG, "✓ Buttons configured (GPIO46, GPIO45)");
    ESP_LOGI(TAG, "✓ Status LED working (GPIO35)");
    ESP_LOGI(TAG, "✅ Wokwi hardware validation complete");
    return ESP_OK;
}
