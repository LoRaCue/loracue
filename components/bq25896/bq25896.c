#include "bq25896.h"
#include "esp_log.h"

static const char *TAG = "bq25896";
static i2c_port_t bq_i2c_port;

#define BQ25896_REG_STATUS  0x0B
#define BQ25896_REG_VBUS    0x11

static esp_err_t bq25896_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_write_read_device(bq_i2c_port, BQ25896_ADDR, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

esp_err_t bq25896_init(i2c_port_t i2c_port)
{
    bq_i2c_port = i2c_port;
    ESP_LOGI(TAG, "Initializing BQ25896 battery charger");
    return ESP_OK;
}

bool bq25896_is_charging(void)
{
    uint8_t status;
    if (bq25896_read_reg(BQ25896_REG_STATUS, &status) == ESP_OK) {
        return (status & 0x18) != 0;
    }
    return false;
}

uint16_t bq25896_get_vbus_mv(void)
{
    uint8_t vbus;
    if (bq25896_read_reg(BQ25896_REG_VBUS, &vbus) == ESP_OK) {
        return (vbus & 0x7F) * 100 + 2600;
    }
    return 0;
}
