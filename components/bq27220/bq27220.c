#include "bq27220.h"
#include "esp_log.h"

static const char *TAG = "bq27220";
static i2c_port_t bq_i2c_port;

#define BQ27220_CMD_SOC 0x1C
#define BQ27220_CMD_VOLTAGE 0x04
#define BQ27220_CMD_CURRENT 0x10

static esp_err_t bq27220_read_word(uint8_t cmd, uint16_t *value)
{
    uint8_t data[2];
    esp_err_t ret = i2c_master_write_read_device(bq_i2c_port, BQ27220_ADDR, &cmd, 1, data, 2, pdMS_TO_TICKS(100));
    if (ret == ESP_OK) {
        *value = data[0] | (data[1] << 8);
    }
    return ret;
}

esp_err_t bq27220_init(i2c_port_t i2c_port)
{
    bq_i2c_port = i2c_port;
    ESP_LOGI(TAG, "Initializing BQ27220 fuel gauge");
    return ESP_OK;
}

uint8_t bq27220_get_soc(void)
{
    uint16_t soc;
    if (bq27220_read_word(BQ27220_CMD_SOC, &soc) == ESP_OK) {
        return soc > 100 ? 100 : soc;
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
