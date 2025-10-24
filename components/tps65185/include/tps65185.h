#pragma once

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TPS65185_ADDR 0x68

esp_err_t tps65185_init(i2c_port_t i2c_port);
esp_err_t tps65185_power_on(void);
esp_err_t tps65185_power_off(void);
esp_err_t tps65185_set_vcom(int16_t vcom_mv);

#ifdef __cplusplus
}
#endif
