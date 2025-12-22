#pragma once

#include "config_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize configuration manager
 */
esp_err_t config_manager_init(void);

/**
 * @brief General configuration operations
 */
esp_err_t config_manager_get_general(general_config_t *config);
esp_err_t config_manager_set_general(const general_config_t *config);

/**
 * @brief Power management configuration operations
 */
esp_err_t config_manager_get_power(power_config_t *config);
esp_err_t config_manager_set_power(const power_config_t *config);

/**
 * @brief LoRa configuration operations
 */
esp_err_t config_manager_get_lora(lora_config_t *config);
esp_err_t config_manager_set_lora(const lora_config_t *config);

/**
 * @brief Regulatory domain operations
 */
esp_err_t config_manager_get_regulatory_domain(char *domain, size_t max_len);
esp_err_t config_manager_set_regulatory_domain(const char *domain);

/**
 * @brief Device registry operations
 */
esp_err_t config_manager_get_device_registry(device_registry_config_t *config);
esp_err_t config_manager_set_device_registry(const device_registry_config_t *config);

/**
 * @brief Atomic transaction support
 */
esp_err_t config_manager_begin_transaction(void);
esp_err_t config_manager_commit_transaction(void);
esp_err_t config_manager_rollback_transaction(void);

/**
 * @brief Validation
 */
esp_err_t config_manager_validate_all(void);

/**
 * @brief Get string representation of device mode
 */
const char *device_mode_to_string(device_mode_t mode);

/**
 * @brief Get device ID derived from MAC address
 */
uint16_t config_manager_get_device_id(void);

/**
 * @brief Factory reset - erase all NVS and reboot
 */
esp_err_t config_manager_factory_reset(void);

#ifdef __cplusplus
}
#endif
