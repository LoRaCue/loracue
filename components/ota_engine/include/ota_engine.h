#pragma once

#include "esp_err.h"
#include <stddef.h>

typedef enum { OTA_STATE_IDLE, OTA_STATE_ACTIVE, OTA_STATE_FINALIZING } ota_state_t;

esp_err_t ota_engine_init(void);
esp_err_t ota_engine_start(size_t firmware_size);
esp_err_t ota_engine_set_expected_sha256(const char *sha256_hex);
esp_err_t ota_engine_write(const uint8_t *data, size_t len);
esp_err_t ota_engine_finish(void);
esp_err_t ota_engine_abort(void);
size_t ota_engine_get_progress(void);
ota_state_t ota_engine_get_state(void);
