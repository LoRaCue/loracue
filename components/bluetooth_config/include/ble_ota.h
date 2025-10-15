#pragma once

#include "esp_err.h"
#include "esp_gatts_api.h"
#include <stdint.h>

// LoRaCue OTA Service UUIDs (128-bit)
// Base UUID: 6E400010-B5A3-F393-E0A9-E50E24DCCA9E
static const uint8_t OTA_SERVICE_UUID[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x10, 0x00, 0x40, 0x6E
};
static const uint8_t OTA_CONTROL_CHAR_UUID[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x11, 0x00, 0x40, 0x6E
};
static const uint8_t OTA_DATA_CHAR_UUID[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x12, 0x00, 0x40, 0x6E
};
static const uint8_t OTA_PROGRESS_CHAR_UUID[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x13, 0x00, 0x40, 0x6E
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
