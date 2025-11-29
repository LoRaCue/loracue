#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool display_sleep_enabled;
    uint32_t display_sleep_timeout_ms;
    bool light_sleep_enabled;
    uint32_t light_sleep_timeout_ms;
    bool deep_sleep_enabled;
    uint32_t deep_sleep_timeout_ms;
} power_mgmt_config_t;

esp_err_t power_mgmt_config_init(void);
esp_err_t power_mgmt_config_get(power_mgmt_config_t *config);
esp_err_t power_mgmt_config_set(const power_mgmt_config_t *config);

#ifdef __cplusplus
}
#endif
