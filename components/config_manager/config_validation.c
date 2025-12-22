#include "config_validation.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "config_validation";

esp_err_t config_validate_general(const general_config_t *config) {
    if (!config) return ESP_ERR_INVALID_ARG;
    
    if (strlen(config->device_name) == 0 || strlen(config->device_name) >= 32) {
        ESP_LOGE(TAG, "Invalid device name length");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->device_mode > 1) {
        ESP_LOGE(TAG, "Invalid device mode: %d", config->device_mode);
        return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_OK;
}

esp_err_t config_validate_power(const power_config_t *config) {
    if (!config) return ESP_ERR_INVALID_ARG;
    
    if (config->display_sleep_timeout_ms < 1000 || config->display_sleep_timeout_ms > 3600000) {
        ESP_LOGE(TAG, "Invalid display sleep timeout: %lu ms", config->display_sleep_timeout_ms);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->light_sleep_timeout_ms < 1000 || config->light_sleep_timeout_ms > 3600000) {
        ESP_LOGE(TAG, "Invalid light sleep timeout: %lu ms", config->light_sleep_timeout_ms);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->deep_sleep_timeout_ms < 10000 || config->deep_sleep_timeout_ms > 86400000) {
        ESP_LOGE(TAG, "Invalid deep sleep timeout: %lu ms", config->deep_sleep_timeout_ms);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->cpu_freq_mhz != 80 && config->cpu_freq_mhz != 160 && config->cpu_freq_mhz != 240) {
        ESP_LOGE(TAG, "Invalid CPU frequency: %d MHz", config->cpu_freq_mhz);
        return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_OK;
}

esp_err_t config_validate_lora(const lora_config_t *config) {
    if (!config) return ESP_ERR_INVALID_ARG;
    
    // Validate frequency ranges (EU868, US915)
    if (!((config->frequency >= 863000000 && config->frequency <= 870000000) ||
          (config->frequency >= 902000000 && config->frequency <= 928000000))) {
        ESP_LOGE(TAG, "Invalid frequency: %lu Hz", config->frequency);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->tx_power < -3 || config->tx_power > 22) {
        ESP_LOGE(TAG, "Invalid TX power: %d dBm", config->tx_power);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->spreading_factor < 6 || config->spreading_factor > 12) {
        ESP_LOGE(TAG, "Invalid spreading factor: %d", config->spreading_factor);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate bandwidth
    if (config->bandwidth != 125 && config->bandwidth != 250 && config->bandwidth != 500) {
        ESP_LOGE(TAG, "Invalid bandwidth: %d kHz", config->bandwidth);
        return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_OK;
}

esp_err_t config_validate_device_registry(const device_registry_config_t *config) {
    if (!config) return ESP_ERR_INVALID_ARG;
    
    if (config->device_count > 32) {
        ESP_LOGE(TAG, "Too many devices: %zu", config->device_count);
        return ESP_ERR_INVALID_ARG;
    }
    
    for (size_t i = 0; i < config->device_count; i++) {
        if (strlen(config->devices[i].device_name) == 0 || 
            strlen(config->devices[i].device_name) >= 32) {
            ESP_LOGE(TAG, "Invalid device name at index %zu", i);
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    return ESP_OK;
}

esp_err_t config_validate_cross_config(const general_config_t *general,
                                       const power_config_t *power,
                                       const lora_config_t *lora,
                                       const device_registry_config_t *registry) {
    esp_err_t ret;
    
    ret = config_validate_general(general);
    if (ret != ESP_OK) return ret;
    
    ret = config_validate_power(power);
    if (ret != ESP_OK) return ret;
    
    ret = config_validate_lora(lora);
    if (ret != ESP_OK) return ret;
    
    ret = config_validate_device_registry(registry);
    if (ret != ESP_OK) return ret;
    
    // Cross-validation rules
    if (general->device_mode == DEVICE_MODE_PRESENTER && registry->device_count == 0) {
        ESP_LOGW(TAG, "Presenter mode with no paired devices");
    }
    
    // Regulatory compliance check (placeholder for future regulatory domain integration)
    if (lora->frequency >= 863000000 && lora->frequency <= 870000000) {
        // EU868 - check duty cycle compliance
        if (lora->tx_power > 14) {
            ESP_LOGW(TAG, "High TX power in EU band may violate duty cycle limits");
        }
    }
    
    return ESP_OK;
}
