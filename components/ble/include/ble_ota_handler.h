/**
 * @file ble_ota_handler.h
 * @brief BLE OTA handler interface
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize BLE OTA handler
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t ble_ota_handler_init(void);

/**
 * @brief Set connection handle for OTA security checks
 * 
 * @param conn_handle BLE connection handle
 */
void ble_ota_handler_set_connection(uint16_t conn_handle);

#ifdef __cplusplus
}
#endif
