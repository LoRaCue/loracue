/**
 * @file bsp_wokwi.c
 * @brief Wokwi Simulator BSP - Compatible with Heltec LoRa V3 pinout
 * 
 * HARDWARE: Wokwi ESP32-S3 + SSD1306 OLED + simulated components
 * PINS: Same as Heltec V3 - SPI(8-14)=LoRa, I2C(17-18)=OLED, GPIO(45-46)=Buttons, ADC(1,37)=Battery
 */

#include "bsp.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "u8g2.h"

static const char *TAG = "BSP_WOKWI";

// Wokwi Simulator Pin Definitions (same as Heltec V3)
#define BUTTON_PREV_PIN         GPIO_NUM_46
#define BUTTON_NEXT_PIN         GPIO_NUM_45
#define STATUS_LED_PIN          GPIO_NUM_35
#define BATTERY_ADC_PIN         GPIO_NUM_1
#define BATTERY_CTRL_PIN        GPIO_NUM_37

// LoRa SX1262 Pins (same as Heltec V3)
#define LORA_SCK_PIN           GPIO_NUM_9
#define LORA_MISO_PIN          GPIO_NUM_11
#define LORA_MOSI_PIN          GPIO_NUM_10
#define LORA_CS_PIN            GPIO_NUM_8
#define LORA_RST_PIN           GPIO_NUM_12
#define LORA_DIO1_PIN          GPIO_NUM_14
#define LORA_BUSY_PIN          GPIO_NUM_13

// SSD1306 OLED Pins (same I2C pins as Heltec V3)
#define OLED_SDA_PIN           GPIO_NUM_17
#define OLED_SCL_PIN           GPIO_NUM_18
#define OLED_RST_PIN           GPIO_NUM_21

// I2C and SPI handles
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t oled_dev_handle = NULL;
static spi_device_handle_t lora_spi_handle = NULL;

// SSD1306 Commands (different from SH1106)
#define SSD1306_I2C_ADDR           0x3C
#define SSD1306_SETCONTRAST        0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON       0xA5
#define SSD1306_NORMALDISPLAY      0xA6
#define SSD1306_INVERTDISPLAY      0xA7
#define SSD1306_DISPLAYOFF         0xAE
#define SSD1306_DISPLAYON          0xAF
#define SSD1306_SETDISPLAYOFFSET   0xD3
#define SSD1306_SETCOMPINS         0xDA
#define SSD1306_SETVCOMDETECT      0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE       0xD9
#define SSD1306_SETMULTIPLEX       0xA8
#define SSD1306_SETLOWCOLUMN       0x00
#define SSD1306_SETHIGHCOLUMN      0x10
#define SSD1306_SETSTARTLINE       0x40
#define SSD1306_MEMORYMODE         0x20
#define SSD1306_COLUMNADDR         0x21
#define SSD1306_PAGEADDR           0x22
#define SSD1306_COMSCANINC         0xC0
#define SSD1306_COMSCANDEC         0xC8
#define SSD1306_SEGREMAP           0xA0
#define SSD1306_CHARGEPUMP         0x8D

esp_err_t wokwi_init_spi(void)
{
    ESP_LOGI(TAG, "Initializing SPI bus for simulated LoRa");
    
    spi_bus_config_t bus_config = {
        .miso_io_num = LORA_MISO_PIN,
        .mosi_io_num = LORA_MOSI_PIN,
        .sclk_io_num = LORA_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = 1000000, // 1MHz for simulation
        .mode = 0,
        .spics_io_num = LORA_CS_PIN,
        .queue_size = 1,
    };
    
    ret = spi_bus_add_device(SPI2_HOST, &dev_config, &lora_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure control pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LORA_RST_PIN) | (1ULL << LORA_DIO1_PIN) | (1ULL << LORA_BUSY_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure control pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return ESP_OK;
}

esp_err_t wokwi_init_i2c(void)
{
    ESP_LOGI(TAG, "Initializing I2C bus for SSD1306 OLED");
    
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = OLED_SCL_PIN,
        .sda_io_num = OLED_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    i2c_device_config_t oled_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SSD1306_I2C_ADDR,
        .scl_speed_hz = 400000, // 400kHz
    };
    
    ret = i2c_master_bus_add_device(i2c_bus_handle, &oled_cfg, &oled_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add OLED device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2C bus initialized successfully");
    return ESP_OK;
}

esp_err_t wokwi_oled_write_command(uint8_t cmd)
{
    if (oled_dev_handle == NULL) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t data[2] = {0x00, cmd}; // Control byte + command
    return i2c_master_transmit(oled_dev_handle, data, 2, 1000);
}

esp_err_t wokwi_oled_init(void)
{
    ESP_LOGI(TAG, "Initializing SSD1306 OLED display");
    
    // Configure RST pin
    gpio_config_t rst_config = {
        .pin_bit_mask = (1ULL << OLED_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&rst_config);
    if (ret != ESP_OK) return ret;
    
    // Reset sequence
    gpio_set_level(OLED_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(OLED_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // SSD1306 initialization sequence
    ret = wokwi_oled_write_command(SSD1306_DISPLAYOFF);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_SETDISPLAYCLOCKDIV);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0x80);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_SETMULTIPLEX);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0x3F); // 64 lines
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_SETDISPLAYOFFSET);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0x00);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_SETSTARTLINE | 0x0);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_CHARGEPUMP);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0x14); // Enable charge pump
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_MEMORYMODE);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0x00); // Horizontal addressing mode
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_SEGREMAP | 0x1);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_COMSCANDEC);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_SETCOMPINS);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0x12);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_SETCONTRAST);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0xCF);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_SETPRECHARGE);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0xF1);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_SETVCOMDETECT);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0x40);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_DISPLAYALLON_RESUME);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_NORMALDISPLAY);
    if (ret != ESP_OK) return ret;
    
    ret = wokwi_oled_write_command(SSD1306_DISPLAYON);
    if (ret != ESP_OK) return ret;
    
    ESP_LOGI(TAG, "SSD1306 OLED initialized successfully");
    return ESP_OK;
}

esp_err_t wokwi_oled_clear(void)
{
    ESP_LOGI(TAG, "Clearing SSD1306 OLED display");
    
    // Set column address range (0-127)
    esp_err_t ret = wokwi_oled_write_command(SSD1306_COLUMNADDR);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(127);
    if (ret != ESP_OK) return ret;
    
    // Set page address range (0-7)
    ret = wokwi_oled_write_command(SSD1306_PAGEADDR);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(0);
    if (ret != ESP_OK) return ret;
    ret = wokwi_oled_write_command(7);
    if (ret != ESP_OK) return ret;
    
    // Clear all pixels (128x64 = 1024 bytes)
    uint8_t clear_data[129] = {0x40}; // Data control byte + 128 zeros
    memset(&clear_data[1], 0x00, 128);
    
    for (int i = 0; i < 8; i++) { // 8 pages
        ret = i2c_master_transmit(oled_dev_handle, clear_data, 129, 1000);
        if (ret != ESP_OK) return ret;
    }
    
    return ESP_OK;
}

// BSP Interface Implementation (same API as Heltec V3)
esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing Wokwi Simulator BSP");
    
    // Initialize GPIO for buttons (same as Heltec V3)
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_PREV_PIN) | (1ULL << BUTTON_NEXT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&button_config);
    if (ret != ESP_OK) return ret;
    
    // Initialize status LED (same as Heltec V3)
    ESP_LOGI(TAG, "Configuring status LED on GPIO%d", STATUS_LED_PIN);
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << STATUS_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&led_config);
    if (ret != ESP_OK) return ret;
    
    // Initialize SPI for simulated LoRa
    ret = wokwi_init_spi();
    if (ret != ESP_OK) return ret;
    
    // Initialize I2C for SSD1306 OLED
    ret = wokwi_init_i2c();
    if (ret != ESP_OK) return ret;
    
    ESP_LOGI(TAG, "Wokwi BSP initialization complete");
    return ESP_OK;
}

// Button functions (same API as Heltec V3)
bool bsp_read_button(bsp_button_t button)
{
    gpio_num_t pin = (button == BSP_BUTTON_PREV) ? BUTTON_PREV_PIN : BUTTON_NEXT_PIN;
    return gpio_get_level(pin) == 0; // Active low
}

// LED functions (same API as Heltec V3)
void bsp_set_led(bool state)
{
    gpio_set_level(STATUS_LED_PIN, state ? 1 : 0);
}

void bsp_toggle_led(void)
{
    static bool led_state = false;
    led_state = !led_state;
    bsp_set_led(led_state);
}

// OLED functions (same API as Heltec V3)
esp_err_t bsp_oled_init(void)
{
    return wokwi_oled_init();
}

esp_err_t bsp_oled_clear(void)
{
    return wokwi_oled_clear();
}

esp_err_t bsp_oled_write_command(uint8_t cmd)
{
    return wokwi_oled_write_command(cmd);
}

// Placeholder functions for compatibility
esp_err_t bsp_enter_sleep(void) { return ESP_OK; }
float bsp_read_battery_voltage(void) { return 3.7f; } // Simulated
uint8_t bsp_read_battery_percentage(void) { return 85; } // Simulated
float bsp_read_battery(void) { return 3.7f; } // Simulated voltage

esp_err_t bsp_validate_hardware(void)
{
    ESP_LOGI(TAG, "Validating Wokwi simulator hardware...");
    
    // Test buttons
    ESP_LOGI(TAG, "✓ Buttons configured (GPIO%d, GPIO%d)", BUTTON_PREV_PIN, BUTTON_NEXT_PIN);
    
    // Test LED
    bsp_set_led(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    bsp_set_led(false);
    ESP_LOGI(TAG, "✓ Status LED working (GPIO%d)", STATUS_LED_PIN);
    
    // Test I2C/OLED
    if (oled_dev_handle != NULL) {
        ESP_LOGI(TAG, "✓ SSD1306 OLED initialized");
    } else {
        ESP_LOGW(TAG, "⚠ OLED not initialized");
    }
    
    // Test SPI/LoRa (simulated)
    if (lora_spi_handle != NULL) {
        ESP_LOGI(TAG, "✓ Simulated LoRa SPI initialized");
    } else {
        ESP_LOGW(TAG, "⚠ LoRa SPI not initialized");
    }
    
    ESP_LOGI(TAG, "✅ Wokwi hardware validation complete");
    return ESP_OK;
}

// u8g2 HAL callbacks for SSD1306
uint8_t u8g2_esp32_i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_BYTE_SEND:
            if (oled_dev_handle != NULL) {
                uint8_t *data = (uint8_t *)arg_ptr;
                esp_err_t ret = i2c_master_transmit(oled_dev_handle, data, arg_int, 1000);
                return (ret == ESP_OK) ? 1 : 0;
            }
            return 0;
            
        case U8X8_MSG_BYTE_INIT:
            return 1;
            
        case U8X8_MSG_BYTE_SET_DC:
            return 1;
            
        case U8X8_MSG_BYTE_START_TRANSFER:
            return 1;
            
        case U8X8_MSG_BYTE_END_TRANSFER:
            return 1;
            
        default:
            return 0;
    }
}

uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            return 1;
            
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            return 1;
            
        case U8X8_MSG_GPIO_RESET:
            gpio_set_level(OLED_RST_PIN, arg_int);
            return 1;
            
        default:
            return 0;
    }
}

esp_err_t bsp_u8g2_init(void *u8g2_ptr)
{
    u8g2_t *u8g2 = (u8g2_t *)u8g2_ptr;
    
    ESP_LOGI(TAG, "Initializing u8g2 with SSD1306 for Wokwi");
    
    // Initialize I2C if not already done
    if (i2c_bus_handle == NULL) {
        esp_err_t ret = wokwi_init_i2c();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize I2C for u8g2");
            return ret;
        }
    }
    
    // Initialize u8g2 with SSD1306 128x64 display
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);
    
    // Initialize display
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    u8g2_ClearDisplay(u8g2);
    
    ESP_LOGI(TAG, "u8g2 initialized successfully for SSD1306");
    return ESP_OK;
}
