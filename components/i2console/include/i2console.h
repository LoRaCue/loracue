/**
 * @file i2console.h
 * @brief I2Console ESP-IDF Component
 * 
 * I2C to USB-CDC console bridge driver for ESP-IDF.
 * Automatically detects I2Console device and routes ESP_LOG output to it.
 */

#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2CONSOLE_DEVICE_ID     0x12C0
#define I2CONSOLE_DEFAULT_ADDR  0x37

/**
 * @brief Initialize I2Console component
 * 
 * Probes I2C bus for I2Console device and registers as ESP-IDF log output.
 * If device not found, silently disables (no error).
 * Uses BSP I2C bus (must be initialized first).
 * 
 * @param addr I2C slave address (default: 0x37)
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if device not detected
 */
esp_err_t i2console_init(uint8_t addr);

/**
 * @brief Write data to I2Console
 * 
 * @param data Data buffer
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t i2console_write(const char *data, size_t len);

/**
 * @brief Check if I2Console is connected
 * 
 * @return true if device detected and operational
 */
bool i2console_is_connected(void);

/**
 * @brief Get I2Console firmware version
 * 
 * @param version Buffer to store version string (min 16 bytes)
 * @return ESP_OK on success
 */
esp_err_t i2console_get_version(char *version);

#ifdef __cplusplus
}
#endif
