#include "bsp.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "BSP_HELTEC_V3";

// Heltec LoRa V3 Pin Definitions
#define LORA_SCK_PIN        GPIO_NUM_9
#define LORA_MISO_PIN       GPIO_NUM_10
#define LORA_MOSI_PIN       GPIO_NUM_11
#define LORA_CS_PIN         GPIO_NUM_8
#define LORA_RST_PIN        GPIO_NUM_12
#define LORA_DIO1_PIN       GPIO_NUM_14
#define LORA_BUSY_PIN       GPIO_NUM_13

#define OLED_SDA_PIN        GPIO_NUM_17
#define OLED_SCL_PIN        GPIO_NUM_18

#define BUTTON_PREV_PIN     GPIO_NUM_46
#define BUTTON_NEXT_PIN     GPIO_NUM_45

#define BATTERY_ADC_PIN     GPIO_NUM_1
#define BATTERY_CTRL_PIN    GPIO_NUM_37

static spi_device_handle_t lora_spi_handle = NULL;
static adc_oneshot_unit_handle_t adc_handle = NULL;

static esp_err_t heltec_v3_init(void)
{
    ESP_LOGI(TAG, "Initializing Heltec LoRa V3 board");
    
    // Initialize GPIO for battery control
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BATTERY_CTRL_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(BATTERY_CTRL_PIN, 0); // Default off to save power
    
    return ESP_OK;
}

static esp_err_t heltec_v3_lora_init(void)
{
    ESP_LOGI(TAG, "Initializing LoRa SPI interface");
    
    spi_bus_config_t buscfg = {
        .miso_io_num = LORA_MISO_PIN,
        .mosi_io_num = LORA_MOSI_PIN,
        .sclk_io_num = LORA_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 256,
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000, // 1MHz
        .mode = 0,
        .spics_io_num = LORA_CS_PIN,
        .queue_size = 1,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &lora_spi_handle));
    
    // Initialize control pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LORA_RST_PIN) | (1ULL << LORA_DIO1_PIN) | (1ULL << LORA_BUSY_PIN),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    return ESP_OK;
}

static esp_err_t heltec_v3_display_init(void)
{
    ESP_LOGI(TAG, "Initializing I2C for OLED display");
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = OLED_SDA_PIN,
        .scl_io_num = OLED_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000, // 400kHz
    };
    
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));
    
    return ESP_OK;
}

static esp_err_t heltec_v3_buttons_init(void)
{
    ESP_LOGI(TAG, "Initializing buttons");
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PREV_PIN) | (1ULL << BUTTON_NEXT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);
    
    return ESP_OK;
}

static float heltec_v3_read_battery(void)
{
    if (!adc_handle) {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
        
        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_12,
            .atten = ADC_ATTEN_DB_11,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &config));
    }
    
    // Enable voltage divider
    gpio_set_level(BATTERY_CTRL_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait for stabilization
    
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &adc_raw));
    
    // Disable voltage divider to save power
    gpio_set_level(BATTERY_CTRL_PIN, 0);
    
    // Convert to voltage: (ADC_raw / 4095) * 3.3V * 4.9 (voltage divider ratio)
    float voltage = ((float)adc_raw / 4095.0f) * 3.3f * 4.9f;
    
    ESP_LOGI(TAG, "Battery voltage: %.2fV (ADC: %d)", voltage, adc_raw);
    return voltage;
}

static void heltec_v3_enter_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep mode");
    
    // Configure wake-up sources (buttons)
    esp_sleep_enable_ext1_wakeup((1ULL << BUTTON_PREV_PIN) | (1ULL << BUTTON_NEXT_PIN), ESP_EXT1_WAKEUP_ANY_LOW);
    
    esp_deep_sleep_start();
}

// Heltec V3 BSP interface
const bsp_interface_t heltec_v3_interface = {
    .init = heltec_v3_init,
    .lora_init = heltec_v3_lora_init,
    .display_init = heltec_v3_display_init,
    .buttons_init = heltec_v3_buttons_init,
    .read_battery = heltec_v3_read_battery,
    .enter_sleep = heltec_v3_enter_sleep,
};
