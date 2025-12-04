#include "bsp.h"
#include "display.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "bsp_lilygo_t3";

// LilyGO T3-S3 Pin Definitions
// Note: Using GPIO0/46 for Wokwi compatibility (real hardware can use GPIO43/44 via external connector)
#define PIN_BUTTON_PREV GPIO_NUM_46
#define PIN_BUTTON_NEXT GPIO_NUM_0

// I2C Pins (for optional peripherals like i2console)
#define PIN_I2C_SDA GPIO_NUM_17
#define PIN_I2C_SCL GPIO_NUM_18

// Rotary Encoder Pins (custom hardware addition)
#define ENCODER_CLK_PIN GPIO_NUM_9
#define ENCODER_DT_PIN GPIO_NUM_10
#define ENCODER_BTN_PIN GPIO_NUM_12

// LED Pin
#define PIN_LED GPIO_NUM_37

// E-Paper Display Pins
#define PIN_EPAPER_MOSI GPIO_NUM_11
#define PIN_EPAPER_CLK GPIO_NUM_14
#define PIN_EPAPER_CS GPIO_NUM_15
#define PIN_EPAPER_DC GPIO_NUM_16
#define PIN_EPAPER_RST GPIO_NUM_47
#define PIN_EPAPER_BUSY GPIO_NUM_48

// LoRa SX1262 Pins
#define PIN_LORA_MISO GPIO_NUM_3
#define PIN_LORA_MOSI GPIO_NUM_6
#define PIN_LORA_SCLK GPIO_NUM_5
#define PIN_LORA_CS GPIO_NUM_7
#define PIN_LORA_RST GPIO_NUM_8
#define PIN_LORA_BUSY GPIO_NUM_34
#define PIN_LORA_DIO1 GPIO_NUM_33

static spi_device_handle_t spi_epaper = NULL;

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing LilyGO T3-S3 BSP");

    // Initialize buttons
    ESP_LOGI(TAG, "Initializing buttons...");
    ESP_ERROR_CHECK(bsp_init_buttons());
    ESP_LOGI(TAG, "Buttons initialized");

    // Initialize I2C for optional peripherals (i2console)
    ESP_LOGI(TAG, "Initializing I2C...");
    ESP_ERROR_CHECK(bsp_i2c_init_default());
    ESP_LOGI(TAG, "I2C initialized");

    // Initialize SPI for E-Paper
    ESP_LOGI(TAG, "Initializing SPI bus for E-Paper...");
    ESP_ERROR_CHECK(
        bsp_spi_init_bus(SPI3_HOST, PIN_EPAPER_MOSI, GPIO_NUM_NC, PIN_EPAPER_CLK, SPI_TRANSFER_SIZE_EPAPER));
    
    // Note: We don't add a device here - the esp_lcd driver will create its own device
    ESP_LOGI(TAG, "SPI bus initialized, esp_lcd will add its own device");

    ESP_LOGI(TAG, "LilyGO T3-S3 BSP initialized");
    return ESP_OK;
}

esp_err_t bsp_deinit(void)
{
    return ESP_OK;
}

esp_err_t bsp_init_buttons(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_BUTTON_PREV) | (1ULL << PIN_BUTTON_NEXT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    return gpio_config(&io_conf);
}

bool bsp_read_button(bsp_button_t button)
{
    gpio_num_t pin = (button == BSP_BUTTON_PREV) ? PIN_BUTTON_PREV : PIN_BUTTON_NEXT;
    return gpio_get_level(pin) == 0;
}

bsp_display_type_t bsp_get_display_type(void)
{
    return BSP_DISPLAY_TYPE_EPAPER_SSD1681;
}

void *bsp_get_spi_device(void)
{
    // For E-Paper displays using esp_lcd, return the SPI host number
    // The esp_lcd driver will create its own device on this bus
    return (void *)(intptr_t)SPI3_HOST;
}

const bsp_epaper_pins_t *bsp_get_epaper_pins(void)
{
    static const bsp_epaper_pins_t pins = {
        .dc = PIN_EPAPER_DC, .cs = PIN_EPAPER_CS, .rst = PIN_EPAPER_RST, .busy = PIN_EPAPER_BUSY};
    return &pins;
}

i2c_master_bus_handle_t bsp_get_i2c_bus(void)
{
    return bsp_i2c_get_bus();
}

esp_err_t bsp_i2c_init_default(void)
{
    return bsp_i2c_init(I2C_NUM_0, PIN_I2C_SDA, PIN_I2C_SCL, 400000);
}

gpio_num_t bsp_get_led_gpio(void)
{
    return PIN_LED;
}

const char *bsp_get_board_id(void)
{
    return "lilygo_t3";
}

const char *bsp_get_model_name(void)
{
    return "LC-Beta";
}

bool bsp_battery_is_charging(void)
{
    // LilyGO T3-S3 doesn't have charging detection hardware
    return false;
}

// cppcheck-suppress unknownMacro
BSP_DEFINE_LORA_PINS(PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_SCLK, PIN_LORA_CS, PIN_LORA_RST, PIN_LORA_BUSY,
                     PIN_LORA_DIO1)

// cppcheck-suppress unknownMacro
// Stub implementations for unused features
BSP_STUB_OK(bsp_init_battery, void)
BSP_STUB_RETURN(bsp_read_battery, float, BSP_STUB_BATTERY_VOLTAGE, void)
BSP_STUB_VOID(bsp_set_led, bool state)
BSP_STUB_VOID(bsp_toggle_led, void)
BSP_STUB_OK(bsp_enter_sleep, void)
BSP_STUB_OK(bsp_init_spi, void)
BSP_STUB_RETURN(bsp_sx1262_read_register, uint8_t, 0, uint16_t reg)
BSP_STUB_OK(bsp_sx1262_reset, void)
BSP_STUB_RETURN(bsp_get_board_name, const char *, "LilyGO T3", void)
BSP_STUB_OK(bsp_validate_hardware, void)
BSP_STUB_RETURN(bsp_get_usb_config, const bsp_usb_config_t *, NULL, void)
BSP_STUB_RETURN(bsp_is_usb_connected, bool, false, void)
BSP_STUB_SERIAL_NUMBER(bsp_get_serial_number)
BSP_STUB_OK(bsp_set_display_contrast, uint8_t contrast)
BSP_STUB_OK(bsp_display_sleep, void)
BSP_STUB_OK(bsp_display_wake, void)
BSP_STUB_RETURN(bsp_get_uart_pins, esp_err_t, ESP_ERR_NOT_SUPPORTED, int uart_num, int *tx_pin, int *rx_pin)
