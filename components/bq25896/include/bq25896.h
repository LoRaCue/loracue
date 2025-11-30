#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BQ25896_ADDR 0x6B

esp_err_t bq25896_init(void);
bool bq25896_is_charging(void);
uint16_t bq25896_get_vbus_mv(void);

#ifdef __cplusplus
}
#endif
