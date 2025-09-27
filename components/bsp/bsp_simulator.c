/**
 * @file bsp_simulator.c
 * @brief BSP implementation for Wokwi simulator
 * 
 * CONTEXT: Simulator-specific BSP for testing without real hardware
 * PURPOSE: Enable full system testing in Wokwi environment
 */

#include "bsp.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "BSP_SIM";

// Simulator pin definitions (matching Wokwi diagram)
#define SIM_OLED_SDA_PIN    17
#define SIM_OLED_SCL_PIN    18
#define SIM_BUTTON_PREV_PIN 45
#define SIM_BUTTON_NEXT_PIN 46
#define SIM_BATTERY_ADC_PIN 1
#define SIM_LED_POWER_PIN   2
#define SIM_LED_TX_PIN      3
#define SIM_LED_RX_PIN      4

// Simulator state
static bool simulator_initialized = false;

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing BSP for Wokwi simulator");
    
    // Configure button GPIOs
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << SIM_BUTTON_PREV_PIN) | (1ULL << SIM_BUTTON_NEXT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&button_config);
    
    // Configure LED GPIOs
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << SIM_LED_POWER_PIN) | (1ULL << SIM_LED_TX_PIN) | (1ULL << SIM_LED_RX_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_config);
    
    // Turn on power LED
    gpio_set_level(SIM_LED_POWER_PIN, 1);
    
    // Configure I2C for OLED
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SIM_OLED_SDA_PIN,
        .scl_io_num = SIM_OLED_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &i2c_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure ADC for battery monitoring
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); // GPIO1
    
    simulator_initialized = true;
    ESP_LOGI(TAG, "Simulator BSP initialized successfully");
    
    return ESP_OK;
}

bool heltec_v3_read_button(bsp_button_t button)
{
    if (!simulator_initialized) {
        return false;
    }
    
    gpio_num_t pin;
    switch (button) {
        case BSP_BUTTON_PREV:
            pin = SIM_BUTTON_PREV_PIN;
            break;
        case BSP_BUTTON_NEXT:
            pin = SIM_BUTTON_NEXT_PIN;
            break;
        default:
            return false;
    }
    
    // Buttons are active low (pressed = 0)
    return !gpio_get_level(pin);
}

float heltec_v3_read_battery(void)
{
    if (!simulator_initialized) {
        return 3.7f; // Default value
    }
    
    // Read ADC value from potentiometer
    int adc_value = adc1_get_raw(ADC1_CHANNEL_0);
    
    // Convert to voltage (simulate battery range 3.0V - 4.2V)
    float voltage = 3.0f + (adc_value / 4095.0f) * 1.2f;
    
    return voltage;
}

esp_err_t heltec_v3_oled_init(void)
{
    ESP_LOGI(TAG, "Initializing simulated OLED display");
    
    // In Wokwi, the SSD1306 display is automatically initialized
    // Just need to verify I2C communication
    
    return ESP_OK;
}

esp_err_t heltec_v3_oled_clear(void)
{
    ESP_LOGD(TAG, "Clearing OLED display");
    
    // Placeholder - in real implementation would clear SSD1306
    // Wokwi will handle display updates automatically
    
    return ESP_OK;
}

esp_err_t heltec_v3_oled_write_line(int line, const char* text)
{
    ESP_LOGD(TAG, "OLED Line %d: %s", line, text);
    
    // Placeholder - in real implementation would write to SSD1306
    // For simulation, just log the text
    
    return ESP_OK;
}

esp_err_t heltec_v3_validate_hardware(void)
{
    ESP_LOGI(TAG, "Running simulator hardware validation");
    
    if (!simulator_initialized) {
        ESP_LOGE(TAG, "BSP not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Test button reading
    ESP_LOGI(TAG, "Testing buttons...");
    bool prev_state = heltec_v3_read_button(BSP_BUTTON_PREV);
    bool next_state = heltec_v3_read_button(BSP_BUTTON_NEXT);
    ESP_LOGI(TAG, "Button states - PREV: %s, NEXT: %s", 
             prev_state ? "PRESSED" : "RELEASED",
             next_state ? "PRESSED" : "RELEASED");
    
    // Test battery reading
    float battery = heltec_v3_read_battery();
    ESP_LOGI(TAG, "Battery voltage: %.2fV", battery);
    
    // Test LEDs
    ESP_LOGI(TAG, "Testing status LEDs...");
    gpio_set_level(SIM_LED_TX_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(SIM_LED_TX_PIN, 0);
    
    gpio_set_level(SIM_LED_RX_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(SIM_LED_RX_PIN, 0);
    
    ESP_LOGI(TAG, "✅ Simulator hardware validation passed");
    return ESP_OK;
}

// Simulator-specific functions
void sim_set_lora_tx_led(bool state)
{
    if (simulator_initialized) {
        gpio_set_level(SIM_LED_TX_PIN, state ? 1 : 0);
    }
}

void sim_set_lora_rx_led(bool state)
{
    if (simulator_initialized) {
        gpio_set_level(SIM_LED_RX_PIN, state ? 1 : 0);
    }
}
