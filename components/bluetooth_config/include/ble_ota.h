#pragma once

#include "esp_err.h"
#include <stdint.h>

esp_err_t ble_ota_init(void);
void ble_ota_handle_control(const uint8_t *data, uint16_t len);
void ble_ota_handle_data(const uint8_t *data, uint16_t len);
