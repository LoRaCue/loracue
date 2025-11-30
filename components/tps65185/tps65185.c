#include "tps65185.h"
#include "bsp.h" // For bsp_i2c_add_device
#include "esp_log.h"

static const char *TAG = "tps65185";
static i2c_master_dev_handle_t tps_handle = NULL;

#define TPS65185_REG_TMST_VALUE 0x00
#define TPS65185_REG_ENABLE 0x01
#define TPS65185_REG_VADJ 0x02
#define TPS65185_REG_VCOM1 0x03
#define TPS65185_REG_VCOM2 0x04
#define TPS65185_REG_INT_EN1 0x05
#define TPS65185_REG_INT_EN2 0x06
#define TPS65185_REG_INT1 0x07
#define TPS65185_REG_INT2 0x08
#define TPS65185_REG_UPSEQ0 0x09
#define TPS65185_REG_UPSEQ1 0x0A
#define TPS65185_REG_DWNSEQ0 0x0B
#define TPS65185_REG_DWNSEQ1 0x0C
#define TPS65185_REG_TMST1 0x0D
#define TPS65185_REG_TMST2 0x0E

static esp_err_t tps65185_write_reg(uint8_t reg, uint8_t value)
{
    if (!tps_handle) return ESP_ERR_INVALID_STATE;
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(tps_handle, data, 2, 100);
}

static esp_err_t tps65185_read_reg(uint8_t reg, uint8_t *value)
{
    if (!tps_handle) return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit_receive(tps_handle, &reg, 1, value, 1, 100);
}

esp_err_t tps65185_init(void)
{
    ESP_LOGI(TAG, "Initializing TPS65185");
    esp_err_t ret = bsp_i2c_add_device(TPS65185_ADDR, 400000, &tps_handle); // Default to 400kHz
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add TPS65185 I2C device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set default VCOM
    tps65185_set_vcom(-2500);

    return ESP_OK;
}

esp_err_t tps65185_power_on(void)
{
    ESP_LOGI(TAG, "Powering on E-Paper rails");
    return tps65185_write_reg(TPS65185_REG_ENABLE, 0xBF);
}

esp_err_t tps65185_power_off(void)
{
    ESP_LOGI(TAG, "Powering off E-Paper rails");
    return tps65185_write_reg(TPS65185_REG_ENABLE, 0x00);
}

esp_err_t tps65185_set_vcom(int16_t vcom_mv)
{
    uint16_t vcom_val = (-vcom_mv) & 0x1FF;
    tps65185_write_reg(TPS65185_REG_VCOM1, vcom_val & 0xFF);
    tps65185_write_reg(TPS65185_REG_VCOM2, (vcom_val >> 8) & 0x01);
    ESP_LOGI(TAG, "Set VCOM to %d mV", vcom_mv);
    return ESP_OK;
}
