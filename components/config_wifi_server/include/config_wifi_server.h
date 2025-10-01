#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start WiFi AP and configuration web server
 * @return ESP_OK on success
 */
esp_err_t config_wifi_server_start(void);

/**
 * @brief Stop WiFi AP and configuration web server
 * @return ESP_OK on success
 */
esp_err_t config_wifi_server_stop(void);

/**
 * @brief Check if configuration server is running
 * @return true if running, false otherwise
 */
bool config_wifi_server_is_running(void);

#ifdef __cplusplus
}
#endif
