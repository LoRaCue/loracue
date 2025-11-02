/**
 * @file bsp_heltec_v3.c
 * @brief Board Support Package for Heltec LoRa V3 (ESP32-S3 + SX1262)
 *
 * CONTEXT: LoRaCue hardware
 * HARDWARE: Heltec LoRa V3 development board
 * DISPLAY: SSD1306 128x64 OLED (not SH1106 as commonly documented)
 * PINS: SPI(8-14)=LoRa, I2C(17-18)=OLED, GPIO(0)=Button, ADC(1,37)=Battery
 * ARCHITECTURE: BSP abstraction layer for multi-board support
 */

#include "bsp.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "u8g2.h"
#include <string.h>

static const char *TAG = "BSP_HELTEC_V3";

// Global u8g2 instance
u8g2_t u8g2;

// Heltec LoRa V3 Pin Definitions
#define BUTTON_PIN GPIO_NUM_0
#define STATUS_LED_PIN GPIO_NUM_35
#define BATTERY_ADC_PIN GPIO_NUM_1
#define BATTERY_CTRL_PIN GPIO_NUM_37
#define VEXT_CTRL_PIN GPIO_NUM_36 // Controls power to OLED and LoRa

// LoRa SX1262 Pins
#define LORA_CS_PIN GPIO_NUM_8
#define LORA_SCK_PIN GPIO_NUM_9
#define LORA_MOSI_PIN GPIO_NUM_10
#define LORA_MISO_PIN GPIO_NUM_11
#define LORA_RST_PIN GPIO_NUM_12
#define LORA_BUSY_PIN GPIO_NUM_13
#define LORA_DIO1_PIN GPIO_NUM_14

// OLED SSD1306 Pins (not SH1106 as commonly documented)
#define OLED_SDA_PIN GPIO_NUM_17
#define OLED_SCL_PIN GPIO_NUM_18
#define OLED_RST_PIN GPIO_NUM_21

// BSP Configuration
#define SPI_CLOCK_SPEED_HZ      1000000     // 1MHz for SX1262
#define SPI_QUEUE_SIZE          1           // Single transaction queue
#define I2C_CLOCK_SPEED_HZ      400000      // 400kHz fast mode
#define ADC_BITWIDTH            ADC_BITWIDTH_12
#define ADC_ATTENUATION         ADC_ATTEN_DB_12

// Static handles
static adc_oneshot_unit_handle_t adc_handle = NULL;
static spi_device_handle_t spi_handle       = NULL;
static SemaphoreHandle_t u8g2_mutex         = NULL;

esp_err_t bsp_init_spi(void)
{
    ESP_LOGD(TAG, "Initializing SPI bus for SX1262 LoRa (MOSI=%d, MISO=%d, SCK=%d, CS=%d)",
             LORA_MOSI_PIN, LORA_MISO_PIN, LORA_SCK_PIN, LORA_CS_PIN);

    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num     = LORA_MOSI_PIN,
        .miso_io_num     = LORA_MISO_PIN,
        .sclk_io_num     = LORA_SCK_PIN,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 256,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure SPI device (SX1262)
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED_HZ,
        .mode           = 0,
        .spics_io_num   = LORA_CS_PIN,
        .queue_size     = SPI_QUEUE_SIZE,
    };

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return ret;
    }

    // Configure control pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LORA_RST_PIN) | (1ULL << LORA_BUSY_PIN) | (1ULL << LORA_DIO1_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    // RST is output, others will be configured as needed
    io_conf.pin_bit_mask = (1ULL << LORA_RST_PIN);
    ret                  = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure RST pin: %s", esp_err_to_name(ret));
        return ret;
    }

    // BUSY and DIO1 as inputs
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << LORA_BUSY_PIN) | (1ULL << LORA_DIO1_PIN);
    ret                  = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure control pins: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return ESP_OK;
}

uint8_t bsp_sx1262_read_register(uint16_t reg)
{
    if (spi_handle == NULL) {
        ESP_LOGE(TAG, "SPI not initialized");
        return 0;
    }

    if (!spi_handle) {
        ESP_LOGE(TAG, "SPI not initialized - spi_handle is NULL");
        return 0;
    }

    // Wait for BUSY to go low
    while (gpio_get_level(LORA_BUSY_PIN)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    uint8_t tx_data[4] = {0x1D, (reg >> 8) & 0xFF, reg & 0xFF, 0x00}; // Read register command
    uint8_t rx_data[4] = {0};

    spi_transaction_t trans = {
        .length    = 32, // 4 bytes * 8 bits
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transaction failed: %s", esp_err_to_name(ret));
        return 0;
    }

    return rx_data[3]; // Register value is in the 4th byte
}

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing Heltec LoRa V3 BSP");

    esp_err_t ret = ESP_OK;

    // Create u8g2 mutex for thread safety
    ESP_LOGD(TAG, "Creating u8g2 mutex");
    u8g2_mutex = xSemaphoreCreateMutex();
    if (!u8g2_mutex) {
        ESP_LOGE(TAG, "Failed to create u8g2 mutex - out of memory");
        return ESP_ERR_NO_MEM;
    }

    // Initialize GPIO for buttons
    ret = bsp_init_buttons();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize buttons: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    // Initialize status LED
    ESP_LOGD(TAG, "Configuring status LED on GPIO%d", STATUS_LED_PIN);
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
        goto cleanup;
    }
    bsp_set_led(false); // Turn off LED initially

    // Initialize battery monitoring
    ret = bsp_init_battery();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize battery monitoring: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    // Initialize SPI for LoRa
    ret = bsp_init_spi();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    // Initialize I2C bus for OLED and future I2C devices
    ret = bsp_i2c_init(OLED_SDA_PIN, OLED_SCL_PIN, I2C_CLOCK_SPEED_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    // Set OLED reset pin
    bsp_oled_set_reset_pin(OLED_RST_PIN);

    // Initialize u8g2 for OLED
    ret = bsp_u8g2_init(&u8g2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize u8g2: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    ESP_LOGI(TAG, "BSP initialization complete");
    return ESP_OK;

cleanup:
    ESP_LOGW(TAG, "Cleaning up after initialization failure");
    bsp_deinit();
    return ret;
}

esp_err_t bsp_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing BSP");
    
    // Clean up SPI
    if (spi_handle) {
        spi_bus_remove_device(spi_handle);
        spi_handle = NULL;
    }
    spi_bus_free(SPI2_HOST);
    
    // Clean up ADC
    if (adc_handle) {
        adc_oneshot_del_unit(adc_handle);
        adc_handle = NULL;
    }
    
    // Clean up I2C
    bsp_i2c_deinit();
    
    // Delete mutex
    if (u8g2_mutex) {
        vSemaphoreDelete(u8g2_mutex);
        u8g2_mutex = NULL;
    }
    
    ESP_LOGI(TAG, "BSP deinitialized");
    return ESP_OK;
}

bool bsp_u8g2_lock(uint32_t timeout_ms)
{
    if (!u8g2_mutex) {
        ESP_LOGW(TAG, "u8g2 mutex not initialized");
        return false;
    }
    return xSemaphoreTake(u8g2_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void bsp_u8g2_unlock(void)
{
    if (u8g2_mutex) {
        xSemaphoreGive(u8g2_mutex);
    }
}

esp_err_t bsp_init_buttons(void)
{
    ESP_LOGI(TAG, "Configuring button GPIO%d", BUTTON_PIN);

    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&button_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Button configured successfully");
    return ESP_OK;
}

esp_err_t bsp_init_battery(void)
{
    ESP_LOGI(TAG, "Initializing battery monitoring on GPIO%d (ADC) with GPIO%d (control)", BATTERY_ADC_PIN,
             BATTERY_CTRL_PIN);

    // Configure battery control pin as output
    gpio_config_t ctrl_config = {
        .pin_bit_mask = (1ULL << BATTERY_CTRL_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&ctrl_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure battery control GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set control pin to high-impedance (power saving)
    gpio_set_level(BATTERY_CTRL_PIN, 0);

    // Initialize ADC
    if (adc_handle == NULL) {
        ESP_LOGD(TAG, "Configuring ADC unit 1, channel 0");
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1,
        };
        ret = adc_oneshot_new_unit(&init_config, &adc_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
            return ret;
        }

        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH,
            .atten    = ADC_ATTENUATION,
        };
        ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
            adc_oneshot_del_unit(adc_handle);
            adc_handle = NULL;
            return ret;
        }
    }

    ESP_LOGI(TAG, "Battery monitoring initialized");
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
    return gpio_get_level(BUTTON_PIN) == 0; // Active low (pulled up, pressed = low)
}

float bsp_read_battery(void)
{
    if (!adc_handle) {
        ESP_LOGE(TAG, "Battery monitoring not initialized - adc_handle is NULL");
        return -1.0f;
    }

    // Enable voltage divider
    gpio_set_level(BATTERY_CTRL_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait for stabilization

    // Take multiple readings and average
    int adc_sum           = 0;
    const int num_samples = 8;

    for (int i = 0; i < num_samples; i++) {
        int adc_raw;
        esp_err_t ret = adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &adc_raw);
        if (ret == ESP_OK) {
            adc_sum += adc_raw;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Disable voltage divider to save power
    gpio_set_level(BATTERY_CTRL_PIN, 0);

    // Calculate voltage: ADC_raw/4095 * 3.3V * voltage_divider_ratio
    // Voltage divider: 390kΩ + 100kΩ = 4.9x multiplier
    float adc_avg = (float)adc_sum / num_samples;
    float voltage = (adc_avg / 4095.0f) * 3.3f * 4.9f;

    ESP_LOGD(TAG, "Battery voltage: %.2fV (ADC: %.0f)", voltage, adc_avg);
    return voltage;
}

esp_err_t bsp_enter_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep, wake on button press");

    // Configure button as wake source
    esp_sleep_enable_ext1_wakeup((1ULL << BUTTON_PIN), ESP_EXT1_WAKEUP_ANY_LOW);

    // Enter deep sleep
    esp_deep_sleep_start();

    // This line should never be reached
    return ESP_OK;
}

esp_err_t bsp_sx1262_reset(void)
{
    ESP_LOGI(TAG, "Resetting SX1262");

    // Pull reset low for 1ms
    gpio_set_level(LORA_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1));

    // Release reset
    gpio_set_level(LORA_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait for chip to boot

    return ESP_OK;
}

// Hardware validation function
esp_err_t bsp_validate_hardware(void)
{
    ESP_LOGI(TAG, "Validating Heltec LoRa V3 hardware");

    // Test battery monitoring
    float battery_voltage = bsp_read_battery();
    if (battery_voltage > 0) {
        ESP_LOGI(TAG, "✓ Battery monitoring working: %.2fV", battery_voltage);
    } else {
        ESP_LOGE(TAG, "✗ Battery monitoring failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "BSP initialization complete");
    return ESP_OK;
}

esp_err_t bsp_u8g2_init(void *u8g2_ptr)
{
    u8g2_t *u8g2_local = (u8g2_t *)u8g2_ptr;

    ESP_LOGI(TAG, "Initializing u8g2 with SSD1306 for Heltec V3");

    // Enable Vext power (powers OLED and LoRa module)
    ESP_LOGI(TAG, "Enabling Vext power for OLED");
    gpio_config_t vext_conf = {.pin_bit_mask = (1ULL << VEXT_CTRL_PIN),
                               .mode         = GPIO_MODE_OUTPUT,
                               .pull_up_en   = GPIO_PULLUP_DISABLE,
                               .pull_down_en = GPIO_PULLDOWN_DISABLE,
                               .intr_type    = GPIO_INTR_DISABLE};
    gpio_config(&vext_conf);
    gpio_set_level(VEXT_CTRL_PIN, 0); // LOW = power ON (active low)
    vTaskDelay(pdMS_TO_TICKS(200));   // Wait for power to stabilize

    // Configure OLED reset pin
    gpio_config_t rst_conf = {.pin_bit_mask = (1ULL << OLED_RST_PIN),
                              .mode         = GPIO_MODE_OUTPUT,
                              .pull_up_en   = GPIO_PULLUP_DISABLE,
                              .pull_down_en = GPIO_PULLDOWN_DISABLE,
                              .intr_type    = GPIO_INTR_DISABLE};
    gpio_config(&rst_conf);

    // Hardware reset sequence for SSD1306
    ESP_LOGI(TAG, "Performing hardware reset on OLED");
    gpio_set_level(OLED_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(OLED_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Wait for I2C bus to stabilize
    vTaskDelay(pdMS_TO_TICKS(200));

    // Initialize u8g2 with SSD1306 128x64 display using BSP callbacks
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2_local, U8G2_R0, bsp_u8g2_i2c_byte_cb, bsp_u8g2_gpio_and_delay_cb);

    // Initialize display
    u8g2_InitDisplay(u8g2_local);
    u8g2_SetPowerSave(u8g2_local, 0);
    u8g2_ClearDisplay(u8g2_local);

    ESP_LOGI(TAG, "u8g2 initialized successfully for SSD1306");
    return ESP_OK;
}

const char *bsp_get_board_id(void)
{
    return "heltec_v3";
}

static const bsp_usb_config_t usb_config = {.usb_pid = 0xFAB0, .usb_product = "LC-alpha"};

const bsp_usb_config_t *bsp_get_usb_config(void)
{
    return &usb_config;
}

esp_err_t bsp_get_serial_number(char *serial_number, size_t max_len)
{
    if (!serial_number || max_len < 13) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(serial_number, max_len, "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return ESP_OK;
}

const bsp_lora_pins_t *bsp_get_lora_pins(void)
{
    static const bsp_lora_pins_t lora_pins = {
        .miso = 11,
        .mosi = 10,
        .sclk = 9,
        .cs = 8,
        .rst = 12,
        .busy = 13,
        .dio1 = 14
    };
    return &lora_pins;
}

esp_err_t bsp_set_display_brightness(uint8_t brightness)
{
    u8g2_SetContrast(&u8g2, brightness);
    return ESP_OK;
}

esp_err_t bsp_display_sleep(void)
{
    u8g2_SetPowerSave(&u8g2, 1);
    return ESP_OK;
}

esp_err_t bsp_display_wake(void)
{
    u8g2_SetPowerSave(&u8g2, 0);
    return ESP_OK;
}

esp_err_t bsp_get_uart_pins(int uart_num, int *tx_pin, int *rx_pin)
{
    if (!tx_pin || !rx_pin) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (uart_num) {
        case 0:
            *tx_pin = 43;  // ESP32-S3 UART0 (USB-JTAG-Serial)
            *rx_pin = 44;
            return ESP_OK;
        case 1:
            *tx_pin = 2;   // Heltec V3 available pins
            *rx_pin = 3;
            return ESP_OK;
        default:
            return ESP_ERR_INVALID_ARG;
    }
}
