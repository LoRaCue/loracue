/**
 * @file ble_ota.h
 * @brief BLE OTA (Over-The-Air) firmware update service interface
 * 
 * Stack-agnostic header that works with both NimBLE and Bluedroid implementations.
 * 
 * @copyright Copyright (c) 2025 LoRaCue Project
 * @license GPL-3.0
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

//==============================================================================
// OTA SERVICE UUIDs (128-bit)
//==============================================================================

// LoRaCue OTA Service UUIDs (128-bit, consecutive)
// Base: 49589A79-7CC5-465D-BFF1-FE37C5065000

// Service: 49589A79-7CC5-465D-BFF1-FE37C5065000
static const uint8_t OTA_SERVICE_UUID[16] = {
    0x00, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
    0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49
};

// Control Characteristic: 49589A79-7CC5-465D-BFF1-FE37C5065001
static const uint8_t OTA_CONTROL_CHAR_UUID[16] = {
    0x01, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
    0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49
};

// Data Characteristic: 49589A79-7CC5-465D-BFF1-FE37C5065002
static const uint8_t OTA_DATA_CHAR_UUID[16] = {
    0x02, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
    0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49
};

// Progress Characteristic: 49589A79-7CC5-465D-BFF1-FE37C5065003
static const uint8_t OTA_PROGRESS_CHAR_UUID[16] = {
    0x03, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
    0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49
};

//==============================================================================
// OTA PROTOCOL DEFINITIONS
//==============================================================================

/**
 * @brief OTA Control Commands (sent to Control characteristic)
 */
#define OTA_CMD_START   0x01  ///< Start OTA (followed by 4-byte size, big-endian)
#define OTA_CMD_ABORT   0x02  ///< Abort OTA transfer
#define OTA_CMD_FINISH  0x03  ///< Finish OTA and reboot device

/**
 * @brief OTA Control Responses (sent from Control characteristic)
 */
#define OTA_RESP_READY    0x10  ///< Ready for data transfer
#define OTA_RESP_ERROR    0x11  ///< Error occurred (followed by error message)
#define OTA_RESP_COMPLETE 0x12  ///< Upload complete, rebooting

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Initialize BLE OTA service
 * 
 * Registers the OTA GATT service with the BLE stack. Must be called after
 * BLE initialization but before starting advertising.
 * 
 * @param conn_handle Initial connection handle (0 if not connected yet)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_ota_service_init(uint16_t conn_handle);

/**
 * @brief Handle write to OTA control characteristic
 * 
 * Processes OTA commands (START, ABORT, FINISH) from the client.
 * 
 * @param data Command data buffer
 * @param len Data length
 */
void ble_ota_handle_control_write(const uint8_t *data, uint16_t len);

/**
 * @brief Handle write to OTA data characteristic
 * 
 * Processes firmware data chunks from the client. Data is written to the
 * OTA partition via the ota_engine module.
 * 
 * @param data Firmware data buffer
 * @param len Data length
 */
void ble_ota_handle_data_write(const uint8_t *data, uint16_t len);

/**
 * @brief Send response over OTA control characteristic
 * 
 * Sends a response code and optional message to the client via indication.
 * 
 * @param response Response code (OTA_RESP_*)
 * @param message Optional null-terminated message string (can be NULL)
 */
void ble_ota_send_response(uint8_t response, const char *message);

/**
 * @brief Update OTA progress
 * 
 * Sends progress percentage to the client via notification on the progress
 * characteristic.
 * 
 * @param percentage Progress percentage (0-100)
 */
void ble_ota_update_progress(uint8_t percentage);

/**
 * @brief Set OTA connection handle
 * 
 * Updates the connection handle used for OTA notifications/indications.
 * Should be called when a new BLE connection is established.
 * 
 * @param conn_handle Connection handle
 */
void ble_ota_set_connection(uint16_t conn_handle);

/**
 * @brief Handle BLE disconnection
 * 
 * Aborts any ongoing OTA transfer and cleans up state. Should be called
 * when the BLE connection is lost.
 */
void ble_ota_handle_disconnect(void);
