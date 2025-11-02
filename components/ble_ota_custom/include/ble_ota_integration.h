/**
 * @file ble_ota_integration.h
 * @brief BLE OTA integration API for existing NimBLE stacks
 * 
 * This header provides functions to integrate BLE OTA functionality
 * into an existing NimBLE stack without reinitializing NimBLE.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register BLE OTA GATT services with existing NimBLE stack
 * 
 * This function registers the OTA GATT services without initializing
 * NimBLE or creating additional tasks. Call this after your NimBLE
 * stack is initialized but before starting the host task.
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t ble_ota_register_services(void);

/**
 * @brief Set connection handle for OTA operations
 * 
 * Call this when a BLE connection is established to enable OTA
 * notifications on that connection.
 * 
 * @param conn_handle BLE connection handle
 */
void ble_ota_set_connection_handle(uint16_t conn_handle);

#ifdef __cplusplus
}
#endif
