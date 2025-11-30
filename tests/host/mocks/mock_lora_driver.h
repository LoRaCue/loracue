#ifndef MOCK_LORA_DRIVER_H
#define MOCK_LORA_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

typedef struct {
    uint32_t frequency;
    uint8_t spreading_factor;
    uint16_t bandwidth;
    int8_t tx_power;
} lora_config_t;

esp_err_t lora_driver_init(void);
esp_err_t lora_send_packet(const uint8_t *data, size_t length);
esp_err_t lora_receive_packet(uint8_t *data, size_t *length, uint32_t timeout_ms);
int16_t lora_get_last_rssi(void);
esp_err_t lora_get_config(lora_config_t *config);

#endif
