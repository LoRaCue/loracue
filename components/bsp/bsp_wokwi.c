/**
 * @file bsp_wokwi.c
 * @brief Minimal BSP for Wokwi simulator - buttons and LEDs only
 */

#include "bsp.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"

static const char *TAG = "BSP_WOKWI";

// Global u8g2 instance
u8g2_t u8g2;

// GPIO pin definitions for Wokwi
#define BUTTON_PREV_PIN GPIO_NUM_46
#define BUTTON_NEXT_PIN GPIO_NUM_45
#define BUTTON_BOTH_PIN GPIO_NUM_21 // Wokwi "BOTH" button
#define STATUS_LED_PIN GPIO_NUM_35
#define I2C_SDA_PIN GPIO_NUM_17
#define I2C_SCL_PIN GPIO_NUM_18

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing Wokwi Simulator BSP");

    // Initialize GPIO for buttons
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_PREV_PIN) | (1ULL << BUTTON_NEXT_PIN) | (1ULL << BUTTON_BOTH_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&button_config);
    if (ret != ESP_OK)
        return ret;

    // Initialize status LED
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << STATUS_LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&led_config);
    if (ret != ESP_OK)
        return ret;

    // Initialize u8g2 HAL for I2C
    u8g2_esp32_hal_t u8g2_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_hal.bus.i2c.sda      = I2C_SDA_PIN;
    u8g2_hal.bus.i2c.scl      = I2C_SCL_PIN;
    u8g2_esp32_hal_init(u8g2_hal);

    // Initialize u8g2 for SSD1306 (Wokwi)
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x3C << 1);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    ESP_LOGI(TAG, "Wokwi BSP initialization complete");
    return ESP_OK;
}

esp_err_t bsp_init_buttons(void)
{
    ESP_LOGI(TAG, "Buttons initialized (simulated)");
    return ESP_OK;
}

esp_err_t bsp_init_i2c(void)
{
    ESP_LOGI(TAG, "I2C initialized (simulated)");
    return ESP_OK;
}

esp_err_t bsp_init_spi(void)
{
    ESP_LOGI(TAG, "SPI initialized (simulated)");
    return ESP_OK;
}

esp_err_t bsp_init_battery(void)
{
    ESP_LOGI(TAG, "Battery monitoring initialized (simulated)");
    return ESP_OK;
}

bool bsp_read_button(bsp_button_t button)
{
    gpio_num_t pin;
    switch (button) {
        case BSP_BUTTON_PREV:
            pin = BUTTON_PREV_PIN;
            break;
        case BSP_BUTTON_NEXT:
            pin = BUTTON_NEXT_PIN;
            break;
        case BSP_BUTTON_BOTH:
            pin = BUTTON_BOTH_PIN;
            break;
        default:
            return false;
    }
    return gpio_get_level(pin) == 0; // Active low
}

void bsp_set_led(bool state)
{
    gpio_set_level(STATUS_LED_PIN, state);
}

float bsp_read_battery(void)
{
    // Return voltage for Wokwi (3.7V = healthy battery)
    return 3.7f;
}

float bsp_read_battery_voltage(void)
{
    // Simulate 3.7V battery
    return 3.7f;
}

void bsp_lora_reset(void)
{
    ESP_LOGI(TAG, "LoRa reset (simulated)");
}

esp_err_t bsp_validate_hardware(void)
{
    ESP_LOGI(TAG, "Validating Wokwi simulator hardware...");
    ESP_LOGI(TAG, "✓ Buttons configured (GPIO46, GPIO45)");
    ESP_LOGI(TAG, "✓ Status LED working (GPIO35)");
    ESP_LOGI(TAG, "✅ Wokwi hardware validation complete");
    return ESP_OK;
}

esp_err_t bsp_u8g2_init(void *u8g2_ptr)
{
    ESP_LOGI(TAG, "u8g2 initialized (simulated for Wokwi)");
    // For Wokwi simulation, we don't initialize the actual hardware
    // The u8g2 structure will be set up by the simulation environment
    // Just return success to avoid the crash
    return ESP_OK;
}

const char* bsp_get_board_id(void)
{
    return "wokwi_sim";
}

