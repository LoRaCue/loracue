#pragma once

#include "esp_err.h"
#include "general_config.h"
#include "power_mgmt_config.h"
#include "lora_driver.h"
#include "device_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

// General Configuration
esp_err_t cmd_get_general_config(general_config_t *config);
esp_err_t cmd_set_general_config(const general_config_t *config);

// Power Management
esp_err_t cmd_get_power_config(power_mgmt_config_t *config);
esp_err_t cmd_set_power_config(const power_mgmt_config_t *config);

// LoRa Configuration
esp_err_t cmd_get_lora_config(lora_config_t *config);
esp_err_t cmd_set_lora_config(const lora_config_t *config);
esp_err_t cmd_set_lora_key(const uint8_t key[32]);

// Device Pairing
esp_err_t cmd_pair_device(const char *name, const uint8_t mac[6], const uint8_t aes_key[32]);
esp_err_t cmd_unpair_device(const uint8_t mac[6]);
esp_err_t cmd_get_paired_devices(paired_device_t *devices, size_t max_count, size_t *count);

// Firmware
esp_err_t cmd_firmware_upgrade_start(size_t size, const char *sha256, const char *signature);

// System
esp_err_t cmd_factory_reset(void);

#ifdef __cplusplus
}
#endif
