/**
 * @file device_registry.h
 * @brief Device registry for managing multiple paired STAGE devices
 *
 * CONTEXT: Ticket 3.2 - Multi-Sender Management
 * PURPOSE: Store and manage paired devices with per-device AES keys
 * STORAGE: NVS for persistent device registry
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_NAME_MAX_LEN 32
#define DEVICE_AES_KEY_LEN 32 // AES-256 requires 32 bytes
#define DEVICE_MAC_ADDR_LEN 6
#define MAX_PAIRED_DEVICES 32  // Optimal for 100KB NVS (~3.5KB storage)

/**
 * @brief Paired device information
 */
typedef struct {
    uint16_t device_id;                       ///< Unique device ID
    char device_name[DEVICE_NAME_MAX_LEN];    ///< User-assigned device name
    uint8_t mac_address[DEVICE_MAC_ADDR_LEN]; ///< Hardware MAC address
    uint8_t aes_key[DEVICE_AES_KEY_LEN];      ///< Per-device AES key
    uint16_t last_sequence;                   ///< Last received sequence number
    uint32_t last_seen;                       ///< Timestamp of last communication
    bool is_active;                           ///< Device entry is valid
} paired_device_t;

/**
 * @brief Initialize device registry
 *
 * @return ESP_OK on success
 */
esp_err_t device_registry_init(void);

/**
 * @brief Add new paired device
 *
 * @param device_id Unique device ID
 * @param device_name User-friendly device name
 * @param mac_address Hardware MAC address
 * @param aes_key Per-device AES-256 encryption key (32 bytes)
 * @return ESP_OK on success, ESP_ERR_NO_MEM if registry full
 */
esp_err_t device_registry_add(uint16_t device_id, const char *device_name, const uint8_t *mac_address,
                              const uint8_t *aes_key);

/**
 * @brief Get device information by ID
 *
 * @param device_id Device ID to lookup
 * @param device Output device information
 * @return ESP_OK if found, ESP_ERR_NOT_FOUND if not found
 */
esp_err_t device_registry_get(uint16_t device_id, paired_device_t *device);

/**
 * @brief Update last seen timestamp and sequence number
 *
 * @param device_id Device ID
 * @param sequence_num Last received sequence number
 * @return ESP_OK on success
 */
esp_err_t device_registry_update_last_seen(uint16_t device_id, uint16_t sequence_num);

/**
 * @brief Remove device from registry
 *
 * @param device_id Device ID to remove
 * @return ESP_OK on success
 */
esp_err_t device_registry_remove(uint16_t device_id);

/**
 * @brief Get list of all paired devices
 *
 * @param devices Array to store device list
 * @param max_devices Maximum number of devices to return
 * @param count Actual number of devices returned
 * @return ESP_OK on success
 */
esp_err_t device_registry_list(paired_device_t *devices, size_t max_devices, size_t *count);

/**
 * @brief Check if device is paired
 *
 * @param device_id Device ID to check
 * @return true if device is paired
 */
bool device_registry_is_paired(uint16_t device_id);

/**
 * @brief Get device count
 *
 * @return Number of paired devices
 */
size_t device_registry_get_count(void);

#ifdef __cplusplus
}
#endif
