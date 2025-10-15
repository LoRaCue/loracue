#pragma once

#include "esp_err.h"
#include "esp_gatts_api.h"
#include <stdint.h>

// OTA Service UUID: 00001234-0000-1000-8000-00805f9b34fb
#define OTA_SERVICE_UUID        0x1234

// OTA Characteristics
#define OTA_CONTROL_CHAR_UUID   0x1235  // Write + Notify - Commands and status
#define OTA_DATA_CHAR_UUID      0x1236  // Write without response - Firmware data
#define OTA_PROGRESS_CHAR_UUID  0x1237  // Read + Notify - Progress percentage

// OTA Control Commands
#define OTA_CMD_START   0x01  // Start OTA (followed by 4-byte size)
#define OTA_CMD_ABORT   0x02  // Abort OTA
#define OTA_CMD_FINISH  0x03  // Finish and reboot

// OTA Control Responses
#define OTA_RESP_READY      0x10  // Ready for data
#define OTA_RESP_ERROR      0x11  // Error occurred
#define OTA_RESP_COMPLETE   0x12  // Upload complete

esp_err_t ble_ota_service_init(esp_gatt_if_t gatts_if);
void ble_ota_handle_control_write(const uint8_t *data, uint16_t len);
void ble_ota_handle_data_write(const uint8_t *data, uint16_t len);
void ble_ota_send_response(uint8_t response, const char *message);
void ble_ota_update_progress(uint8_t percentage);
