#ifndef BLUETOOTH_CONFIG_H
#define BLUETOOTH_CONFIG_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief BLE service data structure
 */
typedef struct {
    uint8_t  version_major;
    uint8_t  version_minor;
    uint8_t  version_patch;
    uint16_t build_flags;
    char     model[];
} __attribute__((packed)) ble_service_data_t;

/**
 * @brief Build flags macros
 */
#define BUILD_NUMBER(n)     ((n) << 2)
#define RELEASE_TYPE_STABLE 0b00
#define RELEASE_TYPE_BETA   0b01
#define RELEASE_TYPE_ALPHA  0b10
#define RELEASE_TYPE_DEV    0b11

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
