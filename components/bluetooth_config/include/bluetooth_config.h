#ifndef BLUETOOTH_CONFIG_H
#define BLUETOOTH_CONFIG_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize Bluetooth configuration service
 *
 * @return ESP_OK on success
 */
esp_err_t bluetooth_config_init(void);

/**
 * @brief Enable/disable Bluetooth
 *
 * @param enabled true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t bluetooth_config_set_enabled(bool enabled);

/**
 * @brief Get Bluetooth enabled status
 *
 * @return true if enabled, false otherwise
 */
bool bluetooth_config_is_enabled(void);

/**
 * @brief Get Bluetooth connection status
 *
 * @return true if connected, false otherwise
 */
bool bluetooth_config_is_connected(void);

/**
 * @brief Get current pairing passkey (if in pairing mode)
 *
 * @param passkey Output buffer for 6-digit passkey
 * @return true if pairing in progress, false otherwise
 */
bool bluetooth_config_get_passkey(uint32_t *passkey);

#endif // BLUETOOTH_CONFIG_H
