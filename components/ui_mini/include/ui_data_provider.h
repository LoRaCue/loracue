#pragma once

#include "esp_err.h"
#include "ui_config.h"

/**
 * @brief Detailed battery information
 */
typedef struct {
    float voltage;      ///< Battery voltage in volts
    uint8_t percentage; ///< Battery percentage (0-100%)
    bool charging;      ///< True if charging via USB
    bool usb_connected; ///< True if USB cable connected
} battery_info_t;

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
const ui_status_t *ui_data_provider_get_status(void);

/**
 * @brief Get detailed battery information
 * @param battery_info Output battery information
 * @return ESP_OK on success
 */
esp_err_t ui_data_provider_get_battery_info(battery_info_t *battery_info);

/**
 * @brief Force update status (for testing/demo)
 * @param usb_connected USB connection status
 * @param lora_connected LoRa connection status
 * @param battery_level Battery level (0-100%)
 * @return ESP_OK on success
 */
esp_err_t ui_data_provider_force_update(bool usb_connected, bool lora_connected, uint8_t battery_level);

/**
 * @brief Reload device name from config (call after SET_GENERAL)
 * @return ESP_OK on success
 */
esp_err_t ui_data_provider_reload_config(void);
