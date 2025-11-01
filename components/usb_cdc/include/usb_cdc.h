#pragma once

#include "esp_err.h"

/**
 * @brief Initialize USB CDC command interface
 * 
 * Creates dedicated task and queue for processing CDC commands.
 * CDC callbacks are registered automatically.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_cdc_init(void);
