#pragma once

#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GT911_ADDR 0x5D

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t size;
    uint8_t id;
} gt911_touch_point_t;

esp_err_t gt911_init(i2c_port_t i2c_port, gpio_num_t int_pin, gpio_num_t rst_pin);
esp_err_t gt911_read_touch(gt911_touch_point_t *point, uint8_t *num_points);

#ifdef __cplusplus
}
#endif
