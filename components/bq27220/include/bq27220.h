#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BQ27220_ADDR 0x55

esp_err_t bq27220_init(void);
uint8_t bq27220_get_soc(void);
uint16_t bq27220_get_voltage_mv(void);
int16_t bq27220_get_current_ma(void);

#ifdef __cplusplus
}
#endif
