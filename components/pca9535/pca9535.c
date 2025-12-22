#include "pca9535.h"
#include "bsp.h" // For bsp_i2c_add_device
#include "esp_log.h"

static const char *TAG = "pca9535";

// PCA9535 registers
#define PCA9535_INPUT_PORT0 0x00
#define PCA9535_INPUT_PORT1 0x01
#define PCA9535_OUTPUT_PORT0 0x02
#define PCA9535_OUTPUT_PORT1 0x03
#define PCA9535_INVERT_PORT0 0x04
#define PCA9535_INVERT_PORT1 0x05
#define PCA9535_CONFIG_PORT0 0x06
#define PCA9535_CONFIG_PORT1 0x07
#define PCA9535_I2C_FREQ_HZ 400000
#define I2C_TIMEOUT_MS 100
#define PCA9535_CONFIG_OUTPUT_ALL 0x00

static i2c_master_dev_handle_t pca_handle = NULL;
static uint8_t pca_addr; // Stored for use by other functions
static uint16_t output_state    = 0;
static uint16_t direction_state = 0xFFFF; // All inputs by default

static esp_err_t pca9535_write_reg(uint8_t reg, uint8_t value)
{
    if (!pca_handle)
        return ESP_ERR_INVALID_STATE;
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(pca_handle, data, 2, I2C_TIMEOUT_MS);
}

static esp_err_t pca9535_read_reg(uint8_t reg, uint8_t *value)
{
    if (!pca_handle)
        return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit_receive(pca_handle, &reg, 1, value, 1, I2C_TIMEOUT_MS);
}

esp_err_t pca9535_init(uint8_t addr)
{
    pca_addr = addr; // Store address for use in helper functions

    ESP_LOGI(TAG, "Initializing PCA9535 at 0x%02X", addr);
    esp_err_t ret = bsp_i2c_add_device(pca_addr, PCA9535_I2C_FREQ_HZ, &pca_handle); // Default to 400kHz
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add PCA9535 I2C device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set all pins as outputs initially
    ret = pca9535_write_reg(PCA9535_CONFIG_PORT0, PCA9535_CONFIG_OUTPUT_ALL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PCA9535");
        return ret;
    }
    ret = pca9535_write_reg(PCA9535_CONFIG_PORT1, PCA9535_CONFIG_OUTPUT_ALL);

    return ret;
}

esp_err_t pca9535_set_direction(pca9535_pin_t pin, bool output)
{
    uint8_t port = (pin >= PCA9535_IO10) ? 1 : 0;
    uint8_t bit  = (pin >= PCA9535_IO10) ? (pin - PCA9535_IO10) : pin;
    uint8_t reg  = (port == 0) ? PCA9535_CONFIG_PORT0 : PCA9535_CONFIG_PORT1;

    if (output) {
        direction_state &= ~(1 << (port * 8 + bit));
    } else {
        direction_state |= (1 << (port * 8 + bit));
    }

    uint8_t value = (port == 0) ? (direction_state & 0xFF) : (direction_state >> 8);
    return pca9535_write_reg(reg, value);
}

esp_err_t pca9535_set_output(pca9535_pin_t pin, uint8_t value)
{
    uint8_t port = (pin >= PCA9535_IO10) ? 1 : 0;
    uint8_t bit  = (pin >= PCA9535_IO10) ? (pin - PCA9535_IO10) : pin;
    uint8_t reg  = (port == 0) ? PCA9535_OUTPUT_PORT0 : PCA9535_OUTPUT_PORT1;

    if (value) {
        output_state |= (1 << (port * 8 + bit));
    } else {
        output_state &= ~(1 << (port * 8 + bit));
    }

    uint8_t out_val = (port == 0) ? (output_state & 0xFF) : (output_state >> 8);
    return pca9535_write_reg(reg, out_val);
}

esp_err_t pca9535_get_input(pca9535_pin_t pin, uint8_t *value)
{
    uint8_t port = (pin >= PCA9535_IO10) ? 1 : 0;
    uint8_t bit  = (pin >= PCA9535_IO10) ? (pin - PCA9535_IO10) : pin;
    uint8_t reg  = (port == 0) ? PCA9535_INPUT_PORT0 : PCA9535_INPUT_PORT1;

    uint8_t port_val;
    esp_err_t ret = pca9535_read_reg(reg, &port_val);
    if (ret == ESP_OK) {
        *value = (port_val >> bit) & 0x01;
    }
    return ret;
}
