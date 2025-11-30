/**
 * @file bsp_i2c.c
 * @brief Centralized I2C bus management for BSP
 */

#include "bsp.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "BSP_I2C";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t bsp_i2c_init(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq_hz)
{
    if (i2c_bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C bus already initialized");
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port                     = port,
        .sda_io_num                   = sda,
        .scl_io_num                   = scl,
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_LOGI(TAG, "Initializing I2C bus: SDA=%d, SCL=%d, freq=%lu Hz", sda, scl, freq_hz);
    return i2c_new_master_bus(&bus_config, &i2c_bus_handle);
}

i2c_master_bus_handle_t bsp_i2c_get_bus(void)
{
    return i2c_bus_handle;
}

esp_err_t bsp_i2c_add_device(uint8_t addr, uint32_t freq_hz, i2c_master_dev_handle_t *dev_handle)
{
    if (i2c_bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = freq_hz,
    };

    ESP_LOGI(TAG, "Adding I2C device: addr=0x%02X, freq=%lu Hz", addr, freq_hz);
    return i2c_master_bus_add_device(i2c_bus_handle, &dev_config, dev_handle);
}

esp_err_t bsp_i2c_deinit(void)
{
    if (i2c_bus_handle) {
        esp_err_t ret = i2c_del_master_bus(i2c_bus_handle);
        if (ret == ESP_OK) {
            i2c_bus_handle = NULL;
            ESP_LOGI(TAG, "I2C bus deinitialized");
        }
        return ret;
    }
    return ESP_OK;
}
