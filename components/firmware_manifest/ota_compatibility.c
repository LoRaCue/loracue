#include "ota_compatibility.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "OTA_COMPAT";

ota_compat_result_t ota_check_compatibility(
    const firmware_manifest_t *new_manifest,
    bool force_mode
)
{
    if (new_manifest == NULL) {
        ESP_LOGE(TAG, "New manifest is NULL");
        return OTA_COMPAT_INVALID_MANIFEST;
    }

    // Validate magic number
    if (new_manifest->magic != FIRMWARE_MAGIC) {
        ESP_LOGE(TAG, "Invalid magic number: 0x%08" PRIX32 " (expected 0x%08" PRIX32 ")",
                 new_manifest->magic, FIRMWARE_MAGIC);
        return OTA_COMPAT_INVALID_MANIFEST;
    }

    // Force mode bypasses all checks
    if (force_mode) {
        ESP_LOGW(TAG, "Force mode enabled - skipping compatibility checks");
        return OTA_COMPAT_OK;
    }

    // Get running firmware manifest
    const firmware_manifest_t *current = firmware_manifest_get();

    // Check board ID compatibility
    if (strncmp(current->board_id, new_manifest->board_id, sizeof(current->board_id)) != 0) {
        ESP_LOGE(TAG, "Board ID mismatch:");
        ESP_LOGE(TAG, "  Current: %s", current->board_id);
        ESP_LOGE(TAG, "  New:     %s", new_manifest->board_id);
        return OTA_COMPAT_BOARD_MISMATCH;
    }

    ESP_LOGI(TAG, "Compatibility check passed:");
    ESP_LOGI(TAG, "  Board ID: %s", current->board_id);
    ESP_LOGI(TAG, "  Current version: %s", current->firmware_version);
    ESP_LOGI(TAG, "  New version:     %s", new_manifest->firmware_version);

    return OTA_COMPAT_OK;
}

esp_err_t ota_extract_manifest(
    const uint8_t *data,
    size_t data_len,
    firmware_manifest_t *manifest
)
{
    if (data == NULL || manifest == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Search for manifest in first 4KB (typical header size)
    const size_t search_len = (data_len < 4096) ? data_len : 4096;
    
    // Look for magic number
    for (size_t i = 0; i <= search_len - sizeof(firmware_manifest_t); i++) {
        const uint32_t *magic_ptr = (const uint32_t *)(data + i);
        
        if (*magic_ptr == FIRMWARE_MAGIC) {
            // Found potential manifest, copy it
            memcpy(manifest, data + i, sizeof(firmware_manifest_t));
            
            // Validate manifest version (uint8_t is always <= 255)
            if (manifest->manifest_version == 0) {
                ESP_LOGW(TAG, "Invalid manifest version: %d", manifest->manifest_version);
                continue;
            }
            
            ESP_LOGI(TAG, "Manifest found at offset %zu:", i);
            ESP_LOGI(TAG, "  Board ID: %s", manifest->board_id);
            ESP_LOGI(TAG, "  Version:  %s", manifest->firmware_version);
            
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "No valid manifest found in firmware binary");
    return ESP_ERR_NOT_FOUND;
}

const char* ota_compat_error_string(ota_compat_result_t result)
{
    switch (result) {
        case OTA_COMPAT_OK:
            return "Compatible";
        case OTA_COMPAT_BOARD_MISMATCH:
            return "Board ID mismatch - wrong hardware";
        case OTA_COMPAT_INVALID_MANIFEST:
            return "Invalid or missing firmware manifest";
        default:
            return "Unknown error";
    }
}
