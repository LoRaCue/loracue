/**
 * @file device_config.h
 * @brief Device configuration management with NVS persistence
 * 
 * CONTEXT: Device configuration and settings management
 * PURPOSE: Store and retrieve device configuration from NVS
 * FEATURES: Settings management, NVS persistence, device behavior
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device mode enumeration
 */
typedef enum {
    DEVICE_MODE_PRESENTER = 0,
    DEVICE_MODE_PC = 1
} device_mode_t;

/**
 * @brief Get string representation of device mode
 * 
 * @param mode Device mode enum value
 * @return String representation of the mode
 */
const char* device_mode_to_string(device_mode_t mode);

/**
 * @brief Device configuration structure (UI and behavior settings)
 */
typedef struct {
    char device_name[32];           ///< Device name
    device_mode_t device_mode;      ///< Current device mode (PRESENTER/PC)
    uint32_t sleep_timeout_ms;      ///< Sleep timeout in milliseconds
    bool auto_sleep_enabled;        ///< Auto sleep enabled
    uint8_t display_brightness;     ///< Display brightness (0-255)
} device_config_t;

/**
 * @brief Initialize device configuration system
 * 
 * @return ESP_OK on success
 */
esp_err_t device_config_init(void);

/**
 * @brief Get current device configuration
 * 
 * @param config Output device configuration
 * @return ESP_OK on success
 */
esp_err_t device_config_get(device_config_t *config);

/**
 * @brief Set device configuration
 * 
 * @param config New device configuration
 * @return ESP_OK on success
 */
esp_err_t device_config_set(const device_config_t *config);

#ifdef __cplusplus
}
#endif
