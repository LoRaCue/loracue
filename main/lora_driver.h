#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the LoRa driver
 * @return ESP_OK on success
 */
esp_err_t lora_driver_init(void);

/**
 * @brief Send a LoRa packet
 * @param data Pointer to data to send
 * @param len Length of data
 * @return ESP_OK on success
 */
esp_err_t lora_send_packet(const uint8_t *data, size_t len);

/**
 * @brief Receive a LoRa packet
 * @param data Buffer to store received data
 * @param max_len Maximum buffer size
 * @param received_len Actual received length
 * @return ESP_OK on success
 */
esp_err_t lora_receive_packet(uint8_t *data, size_t max_len, size_t *received_len);

#ifdef __cplusplus
}
#endif
