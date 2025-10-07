#pragma once

#include "esp_err.h"
#include "lora_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LoRa command received callback
 *
 * @param device_id Sender device ID
 * @param command Received command
 * @param payload Payload data (NULL if no payload)
 * @param payload_length Payload length
 * @param rssi Signal strength in dBm
 * @param user_ctx User context passed during registration
 */
typedef void (*lora_comm_rx_callback_t)(uint16_t device_id, lora_command_t command, const uint8_t *payload,
                                        uint8_t payload_length, int16_t rssi, void *user_ctx);

/**
 * @brief LoRa connection state changed callback
 *
 * @param state New connection state
 * @param user_ctx User context passed during registration
 */
typedef void (*lora_comm_state_callback_t)(lora_connection_state_t state, void *user_ctx);

/**
 * @brief Initialize LoRa communication layer
 *
 * @return ESP_OK on success
 */
esp_err_t lora_comm_init(void);

/**
 * @brief Register callback for received commands
 *
 * @param callback Callback function
 * @param user_ctx User context to pass to callback
 * @return ESP_OK on success
 */
esp_err_t lora_comm_register_rx_callback(lora_comm_rx_callback_t callback, void *user_ctx);

/**
 * @brief Register callback for connection state changes
 *
 * @param callback Callback function
 * @param user_ctx User context to pass to callback
 * @return ESP_OK on success
 */
esp_err_t lora_comm_register_state_callback(lora_comm_state_callback_t callback, void *user_ctx);

/**
 * @brief Start LoRa communication task
 *
 * @return ESP_OK on success
 */
esp_err_t lora_comm_start(void);

/**
 * @brief Stop LoRa communication task
 *
 * @return ESP_OK on success
 */
esp_err_t lora_comm_stop(void);

#ifdef __cplusplus
}
#endif
