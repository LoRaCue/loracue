#pragma once

#include "driver/i2c.h"
#include "esp_err.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCF85063_ADDR 0x51

esp_err_t pcf85063_init(i2c_port_t i2c_port);
esp_err_t pcf85063_set_time(struct tm *time);
esp_err_t pcf85063_get_time(struct tm *time);

#ifdef __cplusplus
}
#endif
