#pragma once

#include "config_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate general configuration
 */
esp_err_t config_validate_general(const general_config_t *config);

/**
 * @brief Validate power configuration
 */
esp_err_t config_validate_power(const power_config_t *config);

/**
 * @brief Validate LoRa configuration
 */
esp_err_t config_validate_lora(const lora_config_t *config);

/**
 * @brief Validate device registry
 */
esp_err_t config_validate_device_registry(const device_registry_config_t *config);

/**
 * @brief Cross-validate all configurations for consistency
 */
esp_err_t config_validate_cross_config(const general_config_t *general,
                                       const power_config_t *power,
                                       const lora_config_t *lora,
                                       const device_registry_config_t *registry);

#ifdef __cplusplus
}
#endif
