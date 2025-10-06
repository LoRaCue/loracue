#pragma once

#include "esp_app_desc.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get board ID from app descriptor
 * Uses project_name field to store board identifier
 */
const char* firmware_manifest_get_board_id(void);

/**
 * @brief Get firmware version from app descriptor
 */
const char* firmware_manifest_get_version(void);

/**
 * @brief Check if OTA image is compatible with current board
 * 
 * @param new_app_info App descriptor from OTA image
 * @return true if compatible, false otherwise
 */
bool firmware_manifest_is_compatible(const esp_app_desc_t *new_app_info);

/**
 * @brief Log firmware manifest information
 */
void firmware_manifest_init(void);

#ifdef __cplusplus
}
#endif
