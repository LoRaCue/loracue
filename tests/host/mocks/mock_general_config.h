#ifndef MOCK_GENERAL_CONFIG_H
#define MOCK_GENERAL_CONFIG_H

#include "esp_err.h"

typedef enum {
    DEVICE_MODE_REMOTE,
    DEVICE_MODE_PC
} device_mode_t;

typedef struct {
    device_mode_t device_mode;
} general_config_t;

esp_err_t general_config_get(general_config_t *config);

#endif
