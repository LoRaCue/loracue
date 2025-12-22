#include "config_manager.h"
#include "config_validation.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// static const char *TAG = "config_manager";

// NVS namespaces
#define NVS_NAMESPACE_GENERAL "general"
#define NVS_NAMESPACE_POWER "power"
#define NVS_NAMESPACE_LORA "lora"
#define NVS_NAMESPACE_REGISTRY "registry"

// Cache
static general_config_t cached_general;
static power_config_t cached_power;
static lora_config_t cached_lora;
static device_registry_config_t cached_registry;
static bool cache_valid[4] = {false, false, false, false};

// Transaction state
static bool transaction_active = false;
static nvs_handle_t transaction_handles[4];

// Default configurations
static const general_config_t default_general = {
    .device_name = "LoRaCue-Device",
    .device_mode = DEVICE_MODE_PRESENTER,
    .display_contrast = 128,
    .bluetooth_enabled = true,
    .bluetooth_pairing_enabled = false,
    .slot_id = 1
};

static const power_config_t default_power = {
    .display_sleep_timeout_ms = 10000,
    .light_sleep_timeout_ms = 30000,
    .deep_sleep_timeout_ms = 300000,
    .enable_auto_display_sleep = true,
    .enable_auto_light_sleep = true,
    .enable_auto_deep_sleep = true,
    .cpu_freq_mhz = 160
};

static const lora_config_t default_lora = {
    .frequency = 868100000,
    .spreading_factor = 7,
    .bandwidth = 500,
    .coding_rate = 5,
    .tx_power = 14,
    .band_id = "HW_868",
    .aes_key = {0}
};

esp_err_t config_manager_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t config_manager_get_general(general_config_t *config) {
    if (cache_valid[0]) {
        *config = cached_general;
        return ESP_OK;
    }

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE_GENERAL, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        *config = default_general;
        return ESP_OK;
    }

    size_t size = sizeof(general_config_t);
    ret = nvs_get_blob(handle, "config", config, &size);
    nvs_close(handle);

    if (ret != ESP_OK) {
        *config = default_general;
    } else {
        cached_general = *config;
        cache_valid[0] = true;
    }
    return ESP_OK;
}

esp_err_t config_manager_set_general(const general_config_t *config) {
    esp_err_t ret = config_validate_general(config);
    if (ret != ESP_OK) return ret;

    nvs_handle_t handle;
    ret = nvs_open(NVS_NAMESPACE_GENERAL, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    ret = nvs_set_blob(handle, "config", config, sizeof(general_config_t));
    if (ret == ESP_OK && !transaction_active) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);

    if (ret == ESP_OK) {
        cached_general = *config;
        cache_valid[0] = true;
    }
    return ret;
}

esp_err_t config_manager_get_power(power_config_t *config) {
    if (cache_valid[1]) {
        *config = cached_power;
        return ESP_OK;
    }

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE_POWER, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        *config = default_power;
        return ESP_OK;
    }

    size_t size = sizeof(power_config_t);
    ret = nvs_get_blob(handle, "config", config, &size);
    nvs_close(handle);

    if (ret != ESP_OK) {
        *config = default_power;
    } else {
        cached_power = *config;
        cache_valid[1] = true;
    }
    return ESP_OK;
}

esp_err_t config_manager_set_power(const power_config_t *config) {
    esp_err_t ret = config_validate_power(config);
    if (ret != ESP_OK) return ret;

    nvs_handle_t handle;
    ret = nvs_open(NVS_NAMESPACE_POWER, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    ret = nvs_set_blob(handle, "config", config, sizeof(power_config_t));
    if (ret == ESP_OK && !transaction_active) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);

    if (ret == ESP_OK) {
        cached_power = *config;
        cache_valid[1] = true;
    }
    return ret;
}

esp_err_t config_manager_get_lora(lora_config_t *config) {
    if (cache_valid[2]) {
        *config = cached_lora;
        return ESP_OK;
    }

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE_LORA, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        *config = default_lora;
        return ESP_OK;
    }

    size_t size = sizeof(lora_config_t);
    ret = nvs_get_blob(handle, "config", config, &size);
    nvs_close(handle);

    if (ret != ESP_OK) {
        *config = default_lora;
    } else {
        cached_lora = *config;
        cache_valid[2] = true;
    }
    return ESP_OK;
}

esp_err_t config_manager_set_lora(const lora_config_t *config) {
    esp_err_t ret = config_validate_lora(config);
    if (ret != ESP_OK) return ret;

    nvs_handle_t handle;
    ret = nvs_open(NVS_NAMESPACE_LORA, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    ret = nvs_set_blob(handle, "config", config, sizeof(lora_config_t));
    if (ret == ESP_OK && !transaction_active) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);

    if (ret == ESP_OK) {
        cached_lora = *config;
        cache_valid[2] = true;
    }
    return ret;
}

esp_err_t config_manager_get_regulatory_domain(char *domain, size_t max_len) {
    lora_config_t config;
    esp_err_t ret = config_manager_get_lora(&config);
    if (ret != ESP_OK) return ret;
    
    strncpy(domain, config.regulatory_domain, max_len - 1);
    domain[max_len - 1] = '\0';
    return ESP_OK;
}

esp_err_t config_manager_set_regulatory_domain(const char *domain) {
    if (!domain || strlen(domain) > 2) return ESP_ERR_INVALID_ARG;
    
    lora_config_t config;
    esp_err_t ret = config_manager_get_lora(&config);
    if (ret != ESP_OK) return ret;
    
    strncpy(config.regulatory_domain, domain, sizeof(config.regulatory_domain) - 1);
    config.regulatory_domain[sizeof(config.regulatory_domain) - 1] = '\0';
    
    return config_manager_set_lora(&config);
}

esp_err_t config_manager_get_device_registry(device_registry_config_t *config) {
    if (cache_valid[3]) {
        *config = cached_registry;
        return ESP_OK;
    }

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE_REGISTRY, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        memset(config, 0, sizeof(device_registry_config_t));
        return ESP_OK;
    }

    size_t size = sizeof(device_registry_config_t);
    ret = nvs_get_blob(handle, "config", config, &size);
    nvs_close(handle);

    if (ret != ESP_OK) {
        memset(config, 0, sizeof(device_registry_config_t));
    } else {
        cached_registry = *config;
        cache_valid[3] = true;
    }
    return ESP_OK;
}

esp_err_t config_manager_set_device_registry(const device_registry_config_t *config) {
    esp_err_t ret = config_validate_device_registry(config);
    if (ret != ESP_OK) return ret;

    nvs_handle_t handle;
    ret = nvs_open(NVS_NAMESPACE_REGISTRY, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    ret = nvs_set_blob(handle, "config", config, sizeof(device_registry_config_t));
    if (ret == ESP_OK && !transaction_active) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);

    if (ret == ESP_OK) {
        cached_registry = *config;
        cache_valid[3] = true;
    }
    return ret;
}

esp_err_t config_manager_begin_transaction(void) {
    if (transaction_active) return ESP_ERR_INVALID_STATE;

    const char* namespaces[] = {NVS_NAMESPACE_GENERAL, NVS_NAMESPACE_POWER, 
                               NVS_NAMESPACE_LORA, NVS_NAMESPACE_REGISTRY};
    
    for (int i = 0; i < 4; i++) {
        esp_err_t ret = nvs_open(namespaces[i], NVS_READWRITE, &transaction_handles[i]);
        if (ret != ESP_OK) {
            for (int j = 0; j < i; j++) {
                nvs_close(transaction_handles[j]);
            }
            return ret;
        }
    }
    
    transaction_active = true;
    return ESP_OK;
}

esp_err_t config_manager_commit_transaction(void) {
    if (!transaction_active) return ESP_ERR_INVALID_STATE;

    esp_err_t ret = ESP_OK;
    for (int i = 0; i < 4; i++) {
        esp_err_t commit_ret = nvs_commit(transaction_handles[i]);
        if (commit_ret != ESP_OK) ret = commit_ret;
        nvs_close(transaction_handles[i]);
    }
    
    transaction_active = false;
    return ret;
}

esp_err_t config_manager_rollback_transaction(void) {
    if (!transaction_active) return ESP_ERR_INVALID_STATE;

    for (int i = 0; i < 4; i++) {
        nvs_close(transaction_handles[i]);
    }
    
    // Invalidate cache to force reload
    memset(cache_valid, false, sizeof(cache_valid));
    transaction_active = false;
    return ESP_OK;
}

esp_err_t config_manager_validate_all(void) {
    general_config_t general;
    power_config_t power;
    lora_config_t lora;
    device_registry_config_t registry;

    esp_err_t ret = config_manager_get_general(&general);
    if (ret != ESP_OK) return ret;
    
    ret = config_manager_get_power(&power);
    if (ret != ESP_OK) return ret;
    
    ret = config_manager_get_lora(&lora);
    if (ret != ESP_OK) return ret;
    
    ret = config_manager_get_device_registry(&registry);
    if (ret != ESP_OK) return ret;

    return config_validate_cross_config(&general, &power, &lora, &registry);
}

esp_err_t config_manager_reset_all(void) {
    esp_err_t ret = config_manager_set_general(&default_general);
    if (ret != ESP_OK) return ret;
    
    ret = config_manager_set_power(&default_power);
    if (ret != ESP_OK) return ret;
    
    ret = config_manager_set_lora(&default_lora);
    if (ret != ESP_OK) return ret;

    device_registry_config_t empty_registry = {0};
    return config_manager_set_device_registry(&empty_registry);
}

const char *device_mode_to_string(device_mode_t mode) {
    switch (mode) {
        case DEVICE_MODE_PRESENTER:
            return "PRESENTER";
        case DEVICE_MODE_PC:
            return "PC";
        default:
            return "UNKNOWN";
    }
}

uint16_t config_manager_get_device_id(void) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    return (mac[4] << 8) | mac[5];
}

esp_err_t config_manager_factory_reset(void) {
    ESP_LOGW("config_manager", "Factory reset initiated - erasing all NVS data");

    esp_err_t ret = nvs_flash_erase();
    if (ret != ESP_OK) {
        ESP_LOGE("config_manager", "Failed to erase NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI("config_manager", "NVS erased successfully, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}
