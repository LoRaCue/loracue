/**
 * @file bsp_wokwi.c
 * @brief Board Support Package for Wokwi Simulator (ESP32-S3)
 *
 * CONTEXT: LoRaCue Wokwi simulation
 * HARDWARE: Wokwi simulator with SSD1306 OLED
 * DIFFERENCES FROM HELTEC V3:
 *   - No LoRa hardware (simulated)
 *   - Three buttons: GPIO0 (main), GPIO46 (second), GPIO21 (both simulator)
 */

#include "bsp.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "u8g2.h"
#include <string.h>

static const char *TAG = "BSP_WOKWI";

// Global u8g2 instance
u8g2_t u8g2;

// Wokwi Pin Definitions (matching Heltec V3 where possible)
#define BUTTON_PIN GPIO_NUM_0       // Main button (same as Heltec V3)
#define BUTTON_SECOND_PIN GPIO_NUM_46  // Second button (Wokwi only)
#define BUTTON_BOTH_PIN GPIO_NUM_21    // Both button simulator (Wokwi only)
#define STATUS_LED_PIN GPIO_NUM_35
#define BATTERY_ADC_PIN GPIO_NUM_1
#define BATTERY_CTRL_PIN GPIO_NUM_37

// OLED SSD1306 Pins (same as Heltec V3)
#define OLED_SDA_PIN GPIO_NUM_17
#define OLED_SCL_PIN GPIO_NUM_18
#define OLED_RST_PIN GPIO_NUM_21

// Static handles
static adc_oneshot_unit_handle_t adc_handle = NULL;

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing Wokwi Simulator BSP");

    esp_err_t ret = ESP_OK;

    // Initialize GPIO for buttons
    ret = bsp_init_buttons();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize buttons: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize status LED
    ESP_LOGI(TAG, "Configuring status LED on GPIO%d", STATUS_LED_PIN);
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << STATUS_LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&led_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    bsp_set_led(false);

    // Initialize battery monitoring (simulated)
    ret = bsp_init_battery();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize battery monitoring: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize SPI (simulated, no actual LoRa)
    ret = bsp_init_spi();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize I2C bus for OLED
    ret = bsp_i2c_init(OLED_SDA_PIN, OLED_SCL_PIN, 400000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set OLED reset pin
    bsp_oled_set_reset_pin(OLED_RST_PIN);

    // Initialize u8g2 for OLED
    ret = bsp_u8g2_init(&u8g2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize u8g2: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "BSP initialization complete");
    return ESP_OK;
}

esp_err_t bsp_init_buttons(void)
{
    ESP_LOGI(TAG, "Configuring buttons: GPIO%d (main), GPIO%d (second), GPIO%d (both)", 
             BUTTON_PIN, BUTTON_SECOND_PIN, BUTTON_BOTH_PIN);

    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_PIN) | (1ULL << BUTTON_SECOND_PIN) | (1ULL << BUTTON_BOTH_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&button_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIOs: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Buttons configured successfully");
    return ESP_OK;
}

esp_err_t bsp_init_battery(void)
{
    ESP_LOGI(TAG, "Initializing battery monitoring (simulated)");
    
    // Simulate ADC initialization
    if (adc_handle == NULL) {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_12,
            .atten    = ADC_ATTEN_DB_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &config));
    }

    ESP_LOGI(TAG, "Battery monitoring initialized");
    return ESP_OK;
}

esp_err_t bsp_init_spi(void)
{
    ESP_LOGI(TAG, "SPI initialized (simulated - no LoRa hardware)");
    return ESP_OK;
}

void bsp_set_led(bool state)
{
    gpio_set_level(STATUS_LED_PIN, state ? 1 : 0);
}

void bsp_toggle_led(void)
{
    static bool led_state = false;
    led_state             = !led_state;
    bsp_set_led(led_state);
}

bool bsp_read_button(bsp_button_t button)
{
    gpio_num_t pin;
    switch (button) {
        case BSP_BUTTON_PREV:
            pin = BUTTON_SECOND_PIN;  // Second button for Wokwi
            break;
        case BSP_BUTTON_NEXT:
            pin = BUTTON_PIN;  // Main button
            break;
        case BSP_BUTTON_BOTH:
            pin = BUTTON_BOTH_PIN;  // Both button simulator
            break;
        default:
            return false;
    }
    return gpio_get_level(pin) == 0; // Active low (pulled up, pressed = low)
}

float bsp_read_battery(void)
{
    // Simulate healthy battery voltage (3.7V)
    return 3.7f;
}

esp_err_t bsp_enter_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep (simulated)");
    
    // Configure buttons as wake sources
    esp_sleep_enable_ext1_wakeup((1ULL << BUTTON_PIN) | (1ULL << BUTTON_SECOND_PIN) | (1ULL << BUTTON_BOTH_PIN), 
                                 ESP_EXT1_WAKEUP_ANY_LOW);
    
    // Enter deep sleep
    esp_deep_sleep_start();
    
    return ESP_OK;
}

esp_err_t bsp_sx1262_reset(void)
{
    ESP_LOGI(TAG, "SX1262 reset (simulated - no LoRa hardware)");
    return ESP_OK;
}

uint8_t bsp_sx1262_read_register(uint16_t reg)
{
    // Simulated register read
    return 0x00;
}

esp_err_t bsp_validate_hardware(void)
{
    ESP_LOGI(TAG, "Validating Wokwi simulator hardware");

    // Test battery monitoring
    float battery_voltage = bsp_read_battery();
    ESP_LOGI(TAG, "✓ Battery monitoring working: %.2fV (simulated)", battery_voltage);

    ESP_LOGI(TAG, "BSP initialization complete");
    return ESP_OK;
}

esp_err_t bsp_u8g2_init(void *u8g2_ptr)
{
    u8g2_t *u8g2_local = (u8g2_t *)u8g2_ptr;

    ESP_LOGI(TAG, "Initializing u8g2 with SSD1306 for Wokwi");

    // Wait for I2C bus to stabilize (important for Wokwi)
    vTaskDelay(pdMS_TO_TICKS(100));

    // Initialize u8g2 with SSD1306 128x64 display using BSP callbacks
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2_local, U8G2_R0, 
                                           bsp_u8g2_i2c_byte_cb, 
                                           bsp_u8g2_gpio_and_delay_cb);

    // Initialize display
    u8g2_InitDisplay(u8g2_local);
    u8g2_SetPowerSave(u8g2_local, 0);
    u8g2_ClearDisplay(u8g2_local);

    ESP_LOGI(TAG, "u8g2 initialized successfully for SSD1306");
    return ESP_OK;
}

const char* bsp_get_board_id(void)
{
    return "wokwi_sim";
}

static const bsp_usb_config_t usb_config = {
    .usb_pid = 0xFAB1,
    .usb_product = "LC-sim"
};

const bsp_usb_config_t* bsp_get_usb_config(void)
{
    return &usb_config;
}

