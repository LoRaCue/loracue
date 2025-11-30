#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the UI subsystem
 *
 * This function initializes the selected UI implementation (ui_mini or ui_rich).
 * The UI will create its own task and subscribe to system events.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_init(void);

/**
 * @brief Deinitialize the UI subsystem
 *
 * Stops the UI task and cleans up resources.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_deinit(void);

#ifdef __cplusplus
}
#endif
