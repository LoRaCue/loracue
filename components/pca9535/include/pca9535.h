#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// PCA9535 pin definitions
typedef enum {
    PCA9535_IO00 = 0,
    PCA9535_IO01,
    PCA9535_IO02,
    PCA9535_IO03,
    PCA9535_IO04,
    PCA9535_IO05,
    PCA9535_IO06,
    PCA9535_IO07,
    PCA9535_IO10,
    PCA9535_IO11,
    PCA9535_IO12,
    PCA9535_IO13,
    PCA9535_IO14,
    PCA9535_IO15,
    PCA9535_IO16,
    PCA9535_IO17
} pca9535_pin_t;

/**
 * @brief Initialize PCA9535 GPIO expander
 * @param addr I2C address (typically 0x20)
 * @return ESP_OK on success
 */
esp_err_t pca9535_init(uint8_t addr);

/**
 * @brief Set pin direction
 * @param pin Pin number
 * @param output true for output, false for input
 * @return ESP_OK on success
 */
esp_err_t pca9535_set_direction(pca9535_pin_t pin, bool output);

/**
 * @brief Set output pin state
 * @param pin Pin number
 * @param value Pin state (0 or 1)
 * @return ESP_OK on success
 */
esp_err_t pca9535_set_output(pca9535_pin_t pin, uint8_t value);

/**
 * @brief Read input pin state
 * @param pin Pin number
 * @param value Pointer to store pin state
 * @return ESP_OK on success
 */
esp_err_t pca9535_get_input(pca9535_pin_t pin, uint8_t *value);

#ifdef __cplusplus
}
#endif
