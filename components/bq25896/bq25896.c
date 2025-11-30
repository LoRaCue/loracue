#include "bq25896.h"
#include "bsp.h"
#include "esp_log.h"

static const char *TAG = "bq25896";
static i2c_master_dev_handle_t bq_handle = NULL;

#define BQ25896_REG_STATUS 0x0B
#define BQ25896_REG_VBUS 0x11

static esp_err_t bq25896_read_reg(uint8_t reg, uint8_t *value)
{
    if (!bq_handle) return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit_receive(bq_handle, &reg, 1, value, 1, 100);
}

esp_err_t bq25896_init(void)
{
    ESP_LOGI(TAG, "Initializing BQ25896 battery charger");
    // Default to 400kHz
    return bsp_i2c_add_device(BQ25896_ADDR, 400000, &bq_handle);
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
