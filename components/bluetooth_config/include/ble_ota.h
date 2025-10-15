#pragma once

#include "esp_err.h"
#include "esp_gatts_api.h"
#include <stdint.h>

// LoRaCue OTA Service UUIDs (128-bit, completely unique)
// Service: 49589A79-7CC5-465D-BFF1-FE37C506502E
static const uint8_t OTA_SERVICE_UUID[16] = {
    0x2E, 0x50, 0x06, 0xC5, 0x37, 0xFE, 0xF1, 0xBF,
    0x5D, 0x46, 0xC5, 0x7C, 0x79, 0x9A, 0x58, 0x49
};
// Control: 4A3C84E8-E2EB-493E-BFA2-6CCB5FAFE772
static const uint8_t OTA_CONTROL_CHAR_UUID[16] = {
    0x72, 0xE7, 0xAF, 0x5F, 0xCB, 0x6C, 0xA2, 0xBF,
    0x3E, 0x49, 0xEB, 0xE2, 0xE8, 0x84, 0x3C, 0x4A
};
// Data: 863C66B8-3E18-4B47-BAD9-E620C3C6CE5C
static const uint8_t OTA_DATA_CHAR_UUID[16] = {
    0x5C, 0xCE, 0xC6, 0xC3, 0x20, 0xE6, 0xD9, 0xBA,
    0x47, 0x4B, 0x18, 0x3E, 0xB8, 0x66, 0x3C, 0x86
};
// Progress: F5946C63-8D74-45C7-8F18-213152813AB8
static const uint8_t OTA_PROGRESS_CHAR_UUID[16] = {
    0xB8, 0x3A, 0x81, 0x52, 0x31, 0x21, 0x18, 0x8F,
    0xC7, 0x45, 0x74, 0x8D, 0x63, 0x6C, 0x94, 0xF5
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
