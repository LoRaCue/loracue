/**
 * @file usb_console.h
 * @brief USB CDC console redirection (debug port)
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize USB console on CDC port 1 (debug)
 * @note Only available in non-release builds
 * @return ESP_OK on success
 */
esp_err_t usb_console_init(void);

#ifdef __cplusplus
}
#endif
