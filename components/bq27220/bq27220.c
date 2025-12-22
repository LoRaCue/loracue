#include "bq27220.h"
#include "bsp.h" // For bsp_i2c_add_device
#include "esp_log.h"

static const char *TAG                   = "bq27220";
static i2c_master_dev_handle_t bq_handle = NULL;

#define BQ27220_CMD_SOC 0x1C
#define BQ27220_CMD_VOLTAGE 0x04
#define BQ27220_CMD_CURRENT 0x10
#define BQ27220_I2C_FREQ_HZ 400000
#define I2C_TIMEOUT_MS 100
#define MAX_SOC 100

static esp_err_t bq27220_read_word(uint8_t cmd, uint16_t *value)
{
    if (!bq_handle)
        return ESP_ERR_INVALID_STATE;
    uint8_t data[2];
    // Need to use transmit_receive to send command and read response
    esp_err_t ret = i2c_master_transmit_receive(bq_handle, &cmd, 1, data, 2, I2C_TIMEOUT_MS);
    if (ret == ESP_OK) {
        *value = data[0] | (data[1] << 8);
    }
    return ret;
}

esp_err_t bq27220_init(void)
{
    ESP_LOGI(TAG, "Initializing BQ27220 fuel gauge");
    // Default to 400kHz
    return bsp_i2c_add_device(BQ27220_ADDR, BQ27220_I2C_FREQ_HZ, &bq_handle);
}

uint8_t bq27220_get_soc(void)
{
    uint16_t soc;
    if (bq27220_read_word(BQ27220_CMD_SOC, &soc) == ESP_OK) {
        return soc > MAX_SOC ? MAX_SOC : soc;
    }
    return 0;
}

uint16_t bq27220_get_voltage_mv(void)
{
    uint16_t voltage;
    if (bq27220_read_word(BQ27220_CMD_VOLTAGE, &voltage) == ESP_OK) {
        return voltage;
    }
    return 0;
}

int16_t bq27220_get_current_ma(void)
{
    uint16_t current;
    if (bq27220_read_word(BQ27220_CMD_CURRENT, &current) == ESP_OK) {
        return (int16_t)current;
    }
    return 0;
}
