#pragma once

#include "esp_err.h"
#include "esp_gatts_api.h"
#include <stdint.h>

// LoRaCue OTA Service UUIDs (128-bit, consecutive)
// Base: 49589A79-7CC5-465D-BFF1-FE37C5065000
// Service: 49589A79-7CC5-465D-BFF1-FE37C5065000
static const uint8_t OTA_SERVICE_UUID[16] = {
    0x00, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
    0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49
};
// Control: 49589A79-7CC5-465D-BFF1-FE37C5065001
static const uint8_t OTA_CONTROL_CHAR_UUID[16] = {
    0x01, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
    0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49
};
// Data: 49589A79-7CC5-465D-BFF1-FE37C5065002
static const uint8_t OTA_DATA_CHAR_UUID[16] = {
    0x02, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
    0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49
};
// Progress: 49589A79-7CC5-465D-BFF1-FE37C5065003
static const uint8_t OTA_PROGRESS_CHAR_UUID[16] = {
    0x03, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
    0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49
};

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
