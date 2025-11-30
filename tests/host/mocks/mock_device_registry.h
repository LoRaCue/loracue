#ifndef MOCK_DEVICE_REGISTRY_H
#define MOCK_DEVICE_REGISTRY_H

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    uint16_t device_id;
    char name[32];
    uint8_t mac[6];
    uint8_t aes_key[32];
} paired_device_t;

esp_err_t device_registry_init(void);
esp_err_t device_registry_get(uint16_t device_id, paired_device_t *device);

#endif
