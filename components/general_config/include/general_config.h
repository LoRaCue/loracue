/**
 * @file general_config.h
 * @brief Device configuration management with NVS persistence
 *
 * CONTEXT: Device configuration and settings management
 * PURPOSE: Store and retrieve device configuration from NVS
 * FEATURES: Settings management, NVS persistence, device behavior
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device mode enumeration
 */
typedef enum { DEVICE_MODE_PRESENTER = 0, DEVICE_MODE_PC = 1 } device_mode_t;

/**
 * @brief Get string representation of device mode
 *
 * @param mode Device mode enum value
 * @return String representation of the mode
 */
const char *device_mode_to_string(device_mode_t mode);

/**
 * @brief Device configuration structure (UI and behavior settings)
 */
typedef struct {
    char device_name[32];           ///< Device name // cppcheck-suppress unusedStructMember
    device_mode_t device_mode;      ///< Current device mode (PRESENTER/PC) // cppcheck-suppress unusedStructMember
    uint8_t display_brightness;     ///< Display brightness (0-255) // cppcheck-suppress unusedStructMember
    bool bluetooth_enabled;         ///< Bluetooth configuration mode enabled // cppcheck-suppress unusedStructMember
    bool bluetooth_pairing_enabled; ///< Bluetooth pairing mode enabled // cppcheck-suppress unusedStructMember
    uint8_t slot_id;                ///< LoRa slot ID (1-16, default=1) // cppcheck-suppress unusedStructMember
} general_config_t;

/**
 * @brief Initialize device configuration system
 *
 * @return ESP_OK on success
 */
esp_err_t general_config_init(void);

/**
 * @brief Get current device configuration
 *
 * @param config Output device configuration
 * @return ESP_OK on success
 */
esp_err_t general_config_get(general_config_t *config);

/**
 * @brief Set device configuration
 *
 * @param config New device configuration
 * @return ESP_OK on success
 */
esp_err_t general_config_set(const general_config_t *config);

/**
 * @brief Get device ID derived from MAC address
 * @return 16-bit device ID (last 2 bytes of MAC)
 */
uint16_t general_config_get_device_id(void);

/**
 * @brief Factory reset - erase all NVS and reboot
 * @return ESP_OK on success (will not return if successful)
 */
esp_err_t general_config_factory_reset(void);

#ifdef __cplusplus
}
#endif
