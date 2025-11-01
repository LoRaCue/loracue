/**
 * @file ble_ota_handler.h
 * @brief BLE OTA handler interface
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize BLE OTA handler
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t ble_ota_handler_init(void);

#ifdef __cplusplus
}
#endif
