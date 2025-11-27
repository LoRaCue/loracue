#include "bsp.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "bsp_lilygo_t3";

// LilyGO T3-S3 Pin Definitions
#define PIN_BUTTON_PREV  GPIO_NUM_43
#define PIN_BUTTON_NEXT  GPIO_NUM_44

// E-Paper Display Pins
#define PIN_EPAPER_MOSI  GPIO_NUM_11
#define PIN_EPAPER_CLK   GPIO_NUM_14
#define PIN_EPAPER_CS    GPIO_NUM_15
#define PIN_EPAPER_DC    GPIO_NUM_16
#define PIN_EPAPER_RST   GPIO_NUM_47
#define PIN_EPAPER_BUSY  GPIO_NUM_48

// LoRa SX1262 Pins
#define PIN_LORA_MISO    GPIO_NUM_3
#define PIN_LORA_MOSI    GPIO_NUM_6
#define PIN_LORA_SCLK    GPIO_NUM_5
#define PIN_LORA_CS      GPIO_NUM_7
#define PIN_LORA_RST     GPIO_NUM_8
#define PIN_LORA_BUSY    GPIO_NUM_34
#define PIN_LORA_DIO1    GPIO_NUM_33

static spi_device_handle_t spi_epaper = NULL;

esp_err_t bsp_init(void) {
    ESP_LOGI(TAG, "Initializing LilyGO T3-S3 BSP");
    
    // Initialize buttons
    ESP_ERROR_CHECK(bsp_init_buttons());
    
    // Initialize SPI for E-Paper
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_EPAPER_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_EPAPER_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_EPAPER_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_epaper));
    
    ESP_LOGI(TAG, "LilyGO T3-S3 BSP initialized");
    return ESP_OK;
}

esp_err_t bsp_deinit(void) {
    return ESP_OK;
}

esp_err_t bsp_init_buttons(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_BUTTON_PREV) | (1ULL << PIN_BUTTON_NEXT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&io_conf);
}

bool bsp_read_button(bsp_button_t button) {
    gpio_num_t pin = (button == BSP_BUTTON_PREV) ? PIN_BUTTON_PREV : PIN_BUTTON_NEXT;
    return gpio_get_level(pin) == 0;
}

bsp_display_type_t bsp_get_display_type(void) {
    return BSP_DISPLAY_TYPE_EPAPER_SSD1681;
}

void *bsp_get_spi_device(void) {
    return spi_epaper;
}

int bsp_get_epaper_dc_pin(void) {
    return PIN_EPAPER_DC;
}

int bsp_get_epaper_cs_pin(void) {
    return PIN_EPAPER_CS;
}

int bsp_get_epaper_rst_pin(void) {
    return PIN_EPAPER_RST;
}

int bsp_get_epaper_busy_pin(void) {
    return PIN_EPAPER_BUSY;
}

i2c_master_bus_handle_t bsp_get_i2c_bus(void) {
    return NULL; // No I2C on T3-S3
}

const char *bsp_get_board_id(void) {
    return "lilygo_t3";
}

const bsp_lora_pins_t *bsp_get_lora_pins(void) {
    static const bsp_lora_pins_t lora_pins = {
        .miso = PIN_LORA_MISO,
        .mosi = PIN_LORA_MOSI,
        .sclk = PIN_LORA_SCLK,
        .cs = PIN_LORA_CS,
        .rst = PIN_LORA_RST,
        .busy = PIN_LORA_BUSY,
        .dio1 = PIN_LORA_DIO1
    };
    return &lora_pins;
}

// Stub implementations for unused features
esp_err_t bsp_init_battery(void) { return ESP_OK; }
float bsp_read_battery(void) { return 4.2f; }
void bsp_set_led(bool state) { (void)state; }
void bsp_toggle_led(void) {}
esp_err_t bsp_enter_sleep(void) { return ESP_OK; }
esp_err_t bsp_init_spi(void) { return ESP_OK; }
uint8_t bsp_sx1262_read_register(uint16_t reg) { (void)reg; return 0; }
esp_err_t bsp_sx1262_reset(void) { return ESP_OK; }
const char* bsp_get_board_name(void) { return "LilyGO T3"; }
esp_err_t bsp_validate_hardware(void) { return ESP_OK; }
const bsp_usb_config_t *bsp_get_usb_config(void) { return NULL; }
bool bsp_is_usb_connected(void) { return false; }
esp_err_t bsp_get_serial_number(char *serial_number, size_t max_len) { 
    snprintf(serial_number, max_len, "T3S3-000000");
    return ESP_OK;
}
esp_err_t bsp_set_display_brightness(uint8_t brightness) { (void)brightness; return ESP_OK; }
esp_err_t bsp_display_sleep(void) { return ESP_OK; }
esp_err_t bsp_display_wake(void) { return ESP_OK; }
esp_err_t bsp_get_uart_pins(int uart_num, int *tx_pin, int *rx_pin) {
    (void)uart_num; (void)tx_pin; (void)rx_pin;
    return ESP_ERR_NOT_SUPPORTED;
}
