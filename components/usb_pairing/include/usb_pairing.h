/**
 * @file usb_pairing.h
 * @brief Simple USB OTG Device-to-Device Pairing Interface
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pairing result callback
 *
 * @param success True if pairing succeeded
 * @param device_id Paired device ID
 * @param device_name Paired device name
 */
typedef void (*usb_pairing_callback_t)(bool success, uint16_t device_id, const char *device_name);

/**
 * @brief Start device-to-device pairing
 *
 * - Presenter mode: Switch to USB host, send PAIR_DEVICE command
 * - PC mode: Stay in device mode, wait for pairing command
 *
 * @param callback Result callback function
 * @return ESP_OK on success
 */
esp_err_t usb_pairing_start(usb_pairing_callback_t callback);

/**
 * @brief Stop pairing process
 *
 * Returns presenter devices to USB device mode.
 *
 * @return ESP_OK on success
 */
esp_err_t usb_pairing_stop(void);

#ifdef __cplusplus
}
#endif
