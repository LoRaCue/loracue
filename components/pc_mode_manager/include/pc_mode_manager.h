/**
 * @file pc_mode_manager.h
 * @brief PC Mode Manager - handles LoRa commands in PC mode
 *
 * CONTEXT: Manages active presenters, command history, and rate limiting
 * PURPOSE: Separate PC mode business logic from main.c
 */

#pragma once

#include "esp_err.h"
#include "lora_protocol.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize PC mode manager
 *
 * @return ESP_OK on success
 */
esp_err_t pc_mode_manager_init(void);

/**
 * @brief Process incoming LoRa command in PC mode
 *
 * @param device_id Source device ID
 * @param sequence_num Sequence number
 * @param command Command type
 * @param payload Command payload
 * @param payload_length Payload length
 * @param rssi Signal strength
 * @return ESP_OK if processed, ESP_ERR_INVALID_STATE if rate limited
 */
esp_err_t pc_mode_manager_process_command(uint16_t device_id, uint16_t sequence_num, lora_command_t command,
                                          const uint8_t *payload, uint8_t payload_length, int16_t rssi);

/**
 * @brief Deinitialize PC mode manager
 */
void pc_mode_manager_deinit(void);

#ifdef __cplusplus
}
#endif
