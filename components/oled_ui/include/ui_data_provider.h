#pragma once

#include "ui_config.h"
#include "esp_err.h"

/**
 * @brief Initialize UI data provider
 * @return ESP_OK on success
 */
esp_err_t ui_data_provider_init(void);

/**
 * @brief Update status from BSP sensors
 * @return ESP_OK on success
 */
esp_err_t ui_data_provider_update(void);

/**
 * @brief Get current UI status
 * @return Pointer to current status, NULL on error
 */
const ui_status_t* ui_data_provider_get_status(void);

/**
 * @brief Force update status (for testing/demo)
 * @param usb_connected USB connection status
 * @param lora_connected LoRa connection status  
 * @param battery_level Battery level (0-100%)
 * @return ESP_OK on success
 */
esp_err_t ui_data_provider_force_update(bool usb_connected, bool lora_connected, uint8_t battery_level);
