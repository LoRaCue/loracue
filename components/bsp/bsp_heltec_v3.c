/**
 * @file bsp_heltec_v3.c
 * @brief Board Support Package for Heltec LoRa V3 (ESP32-S3 + SX1262)
 * 
 * CONTEXT: LoRaCue enterprise presentation clicker
 * HARDWARE: Heltec LoRa V3 development board
 * PINS: SPI(8-14)=LoRa, I2C(17-18)=OLED, GPIO(45-46)=Buttons, ADC(1,37)=Battery
 * PROTOCOL: SF7/BW500kHz LoRa with AES-128 encryption
 * ARCHITECTURE: BSP abstraction layer for multi-board support
 */

#include "bsp.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include <string.h>

static const char *TAG = "BSP_HELTEC_V3";

// Heltec LoRa V3 Pin Definitions
#define BUTTON_PREV_PIN         GPIO_NUM_46
#define BUTTON_NEXT_PIN         GPIO_NUM_45
#define BATTERY_ADC_PIN         GPIO_NUM_1
#define BATTERY_CTRL_PIN        GPIO_NUM_37

// LoRa SX1262 Pins
#define LORA_MOSI_PIN          GPIO_NUM_11
#define LORA_MISO_PIN          GPIO_NUM_10
#define LORA_SCK_PIN           GPIO_NUM_9
#define LORA_CS_PIN            GPIO_NUM_8
#define LORA_BUSY_PIN          GPIO_NUM_13
#define LORA_DIO1_PIN          GPIO_NUM_14
#define LORA_RST_PIN           GPIO_NUM_12

// OLED SH1106 Pins
#define OLED_SDA_PIN           GPIO_NUM_17
#define OLED_SCL_PIN           GPIO_NUM_18

// Static handles
static adc_oneshot_unit_handle_t adc_handle = NULL;
static spi_device_handle_t spi_handle = NULL;
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t oled_dev_handle = NULL;

esp_err_t heltec_v3_init_spi(void)
{
    ESP_LOGI(TAG, "Initializing SPI bus for SX1262 LoRa");
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = LORA_MOSI_PIN,
        .miso_io_num = LORA_MISO_PIN,
        .sclk_io_num = LORA_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 256,
    };
    
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure SPI device (SX1262)
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,  // 1MHz
        .mode = 0,                  // SPI mode 0
        .spics_io_num = LORA_CS_PIN,
        .queue_size = 1,
    };
    
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure control pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LORA_RST_PIN) | (1ULL << LORA_BUSY_PIN) | (1ULL << LORA_DIO1_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    // RST is output, others will be configured as needed
    io_conf.pin_bit_mask = (1ULL << LORA_RST_PIN);
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure RST pin: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // BUSY and DIO1 as inputs
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << LORA_BUSY_PIN) | (1ULL << LORA_DIO1_PIN);
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure control pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return ESP_OK;
}

uint8_t heltec_v3_sx1262_read_register(uint16_t reg)
{
    if (spi_handle == NULL) {
        ESP_LOGE(TAG, "SPI not initialized");
        return 0;
    }
    
    // Wait for BUSY to go low
    while (gpio_get_level(LORA_BUSY_PIN)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    uint8_t tx_data[4] = {0x1D, (reg >> 8) & 0xFF, reg & 0xFF, 0x00}; // Read register command
    uint8_t rx_data[4] = {0};
    
    spi_transaction_t trans = {
        .length = 32,  // 4 bytes * 8 bits
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

esp_err_t heltec_v3_init_i2c(void)
{
    ESP_LOGI(TAG, "Initializing I2C bus for SH1106 OLED");
    
    // Configure I2C master bus
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = OLED_SCL_PIN,
        .sda_io_num = OLED_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure SH1106 device
    i2c_device_config_t oled_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x3C, // SH1106 I2C address
        .scl_speed_hz = 400000,  // 400kHz
    };
    
    ret = i2c_master_bus_add_device(i2c_bus_handle, &oled_cfg, &oled_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add OLED device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2C bus initialized successfully");
    return ESP_OK;
}

esp_err_t heltec_v3_oled_write_command(uint8_t cmd)
{
    if (oled_dev_handle == NULL) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t data[2] = {0x00, cmd}; // Control byte + command
    return i2c_master_transmit(oled_dev_handle, data, 2, 1000);
}

esp_err_t heltec_v3_oled_init(void)
{
    ESP_LOGI(TAG, "Initializing SH1106 OLED display");
    
    // SH1106 initialization sequence
    esp_err_t ret;
    
    ret = heltec_v3_oled_write_command(0xAE); // Display OFF
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0x02); // Set lower column address
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0x10); // Set higher column address
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0x40); // Set display start line
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0x81); // Set contrast control
    if (ret != ESP_OK) return ret;
    ret = heltec_v3_oled_write_command(0xCF); // Contrast value
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xA1); // Set segment re-map
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xC8); // Set COM output scan direction
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xA6); // Set normal display
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xA8); // Set multiplex ratio
    if (ret != ESP_OK) return ret;
    ret = heltec_v3_oled_write_command(0x3F); // 64 lines
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xA4); // Set entire display ON/OFF
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xD3); // Set display offset
    if (ret != ESP_OK) return ret;
    ret = heltec_v3_oled_write_command(0x00); // No offset
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xD5); // Set display clock
    if (ret != ESP_OK) return ret;
    ret = heltec_v3_oled_write_command(0x80); // Default clock
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xD9); // Set pre-charge period
    if (ret != ESP_OK) return ret;
    ret = heltec_v3_oled_write_command(0xF1); // Pre-charge value
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xDA); // Set COM pins configuration
    if (ret != ESP_OK) return ret;
    ret = heltec_v3_oled_write_command(0x12); // COM pins config
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xDB); // Set VCOM deselect level
    if (ret != ESP_OK) return ret;
    ret = heltec_v3_oled_write_command(0x40); // VCOM level
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0x20); // Set memory addressing mode
    if (ret != ESP_OK) return ret;
    ret = heltec_v3_oled_write_command(0x02); // Page addressing mode
    if (ret != ESP_OK) return ret;
    
    ret = heltec_v3_oled_write_command(0xAF); // Display ON
    if (ret != ESP_OK) return ret;
    
    ESP_LOGI(TAG, "SH1106 OLED initialized successfully");
    return ESP_OK;
}

esp_err_t heltec_v3_oled_clear(void)
{
    ESP_LOGI(TAG, "Clearing OLED display");
    
    for (int page = 0; page < 8; page++) {
        // Set page address
        esp_err_t ret = heltec_v3_oled_write_command(0xB0 + page);
        if (ret != ESP_OK) return ret;
        
        // Set column address
        ret = heltec_v3_oled_write_command(0x02); // Lower column
        if (ret != ESP_OK) return ret;
        ret = heltec_v3_oled_write_command(0x10); // Higher column
        if (ret != ESP_OK) return ret;
        
        // Clear page data
        uint8_t clear_data[130] = {0x40}; // Data control byte + 128 zeros
        memset(&clear_data[1], 0x00, 128);
        
        ret = i2c_master_transmit(oled_dev_handle, clear_data, 129, 1000);
        if (ret != ESP_OK) return ret;
    }
    
    return ESP_OK;
}

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing Heltec LoRa V3 BSP");
    
    esp_err_t ret = ESP_OK;
    
    // Initialize GPIO for buttons
    ret = heltec_v3_init_buttons();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize buttons: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize battery monitoring
    ret = heltec_v3_init_battery();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize battery monitoring: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize SPI for LoRa
    ret = heltec_v3_init_spi();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize I2C for OLED
    ret = heltec_v3_init_i2c();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "BSP initialization complete");
    return ESP_OK;
}

esp_err_t heltec_v3_init_buttons(void)
{
    ESP_LOGI(TAG, "Configuring buttons GPIO%d (PREV) and GPIO%d (NEXT)", BUTTON_PREV_PIN, BUTTON_NEXT_PIN);
    
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_PREV_PIN) | (1ULL << BUTTON_NEXT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&button_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIOs: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Buttons configured successfully");
    return ESP_OK;
}

esp_err_t heltec_v3_init_battery(void)
{
    ESP_LOGI(TAG, "Initializing battery monitoring on GPIO%d (ADC) with GPIO%d (control)", BATTERY_ADC_PIN, BATTERY_CTRL_PIN);
    
    // Configure battery control pin as output
    gpio_config_t ctrl_config = {
        .pin_bit_mask = (1ULL << BATTERY_CTRL_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
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
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
        
        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_12,
            .atten = ADC_ATTEN_DB_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &config));
    }
    
    ESP_LOGI(TAG, "Battery monitoring initialized");
    return ESP_OK;
}

bool heltec_v3_read_button(bsp_button_t button)
{
    gpio_num_t pin = (button == BSP_BUTTON_PREV) ? BUTTON_PREV_PIN : BUTTON_NEXT_PIN;
    return gpio_get_level(pin) == 0; // Active low (pulled up, pressed = low)
}

float heltec_v3_read_battery(void)
{
    if (adc_handle == NULL) {
        ESP_LOGE(TAG, "Battery monitoring not initialized");
        return -1.0f;
    }
    
    // Enable voltage divider
    gpio_set_level(BATTERY_CTRL_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait for stabilization
    
    // Take multiple readings and average
    int adc_sum = 0;
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

esp_err_t heltec_v3_enter_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep, wake on button press");
    
    // Configure both buttons as wake sources
    esp_sleep_enable_ext1_wakeup((1ULL << BUTTON_PREV_PIN) | (1ULL << BUTTON_NEXT_PIN), ESP_EXT1_WAKEUP_ANY_LOW);
    
    // Enter deep sleep
    esp_deep_sleep_start();
    
    // This line should never be reached
    return ESP_OK;
}

esp_err_t heltec_v3_sx1262_reset(void)
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
esp_err_t heltec_v3_validate_hardware(void)
{
    ESP_LOGI(TAG, "Validating Heltec LoRa V3 hardware");
    
    // Test SH1106 OLED display
    ESP_LOGI(TAG, "Testing SH1106 OLED display...");
    esp_err_t ret = heltec_v3_oled_init();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ SH1106 OLED initialized successfully");
        heltec_v3_oled_clear();
        ESP_LOGI(TAG, "✓ OLED display cleared");
    } else {
        ESP_LOGW(TAG, "⚠ SH1106 OLED initialization failed: %s", esp_err_to_name(ret));
    }
    
    // Test SX1262 communication
    ESP_LOGI(TAG, "Testing SX1262 LoRa chip...");
    heltec_v3_sx1262_reset();
    
    // Read version register (should return 0x14 for SX1262)
    uint8_t version = heltec_v3_sx1262_read_register(0x0320);
    if (version == 0x14) {
        ESP_LOGI(TAG, "✓ SX1262 detected (version: 0x%02X)", version);
    } else {
        ESP_LOGW(TAG, "⚠ SX1262 version unexpected: 0x%02X (expected 0x14)", version);
    }
    
    // Test button functionality
    ESP_LOGI(TAG, "Testing buttons - press PREV and NEXT buttons now...");
    
    bool prev_pressed = false, next_pressed = false;
    int timeout = 50; // 5 seconds
    
    while (timeout-- > 0 && (!prev_pressed || !next_pressed)) {
        if (heltec_v3_read_button(BSP_BUTTON_PREV)) {
            if (!prev_pressed) {
                ESP_LOGI(TAG, "✓ PREV button (GPIO%d) working", BUTTON_PREV_PIN);
                prev_pressed = true;
            }
        }
        if (heltec_v3_read_button(BSP_BUTTON_NEXT)) {
            if (!next_pressed) {
                ESP_LOGI(TAG, "✓ NEXT button (GPIO%d) working", BUTTON_NEXT_PIN);
                next_pressed = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (!prev_pressed || !next_pressed) {
        ESP_LOGW(TAG, "Button test incomplete - PREV:%s NEXT:%s", 
                prev_pressed ? "OK" : "FAIL", next_pressed ? "OK" : "FAIL");
    }
    
    // Test battery monitoring
    float battery_voltage = heltec_v3_read_battery();
    if (battery_voltage > 0) {
        ESP_LOGI(TAG, "✓ Battery monitoring working: %.2fV", battery_voltage);
    } else {
        ESP_LOGE(TAG, "✗ Battery monitoring failed");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Hardware validation complete");
    return ESP_OK;
}
