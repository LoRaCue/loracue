#pragma once

#include "firmware_manifest.h"
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTA compatibility check results
 */
typedef enum {
    OTA_COMPAT_OK = 0,              ///< Firmware is compatible
    OTA_COMPAT_BOARD_MISMATCH,      ///< Board ID mismatch
    OTA_COMPAT_INVALID_MANIFEST,    ///< Invalid or missing manifest
} ota_compat_result_t;

/**
 * @brief Check OTA firmware compatibility
 * 
 * Compares new firmware manifest against running firmware.
 * 
 * @param new_manifest Pointer to new firmware manifest
 * @param force_mode If true, skip compatibility checks
 * @return OTA_COMPAT_OK if compatible, error code otherwise
 */
ota_compat_result_t ota_check_compatibility(
    const firmware_manifest_t *new_manifest,
    bool force_mode
);

/**
 * @brief Extract firmware manifest from binary data
 * 
 * Searches for manifest in first 4KB of firmware binary.
 * 
 * @param data Pointer to firmware binary data
 * @param data_len Length of data buffer
 * @param manifest Output buffer for extracted manifest
 * @return ESP_OK if manifest found and valid, error otherwise
 */
esp_err_t ota_extract_manifest(
    const uint8_t *data,
    size_t data_len,
    firmware_manifest_t *manifest
);

/**
 * @brief Get compatibility error description
 * 
 * @param result Compatibility check result
 * @return Human-readable error description
 */
const char* ota_compat_error_string(ota_compat_result_t result);

#ifdef __cplusplus
}
#endif
