/**
 * @file lora_driver.h
 * @brief LoRa driver wrapper for LoRaCue presentation clicker
 *
 * CONTEXT: LoRaCue Phase 2 - Core Communication
 * HARDWARE: SX1262 LoRa transceiver via SPI
 * CONFIG: SF7/BW500kHz/CR4:5 for <50ms latency
 * PURPOSE: Point-to-point LoRa communication wrapper
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LoRa configuration for low latency
 */
typedef struct {
    uint32_t frequency;       ///< Frequency in Hz (e.g., 915000000 for 915MHz)
    uint8_t spreading_factor; ///< SF7 for low latency
    uint16_t bandwidth;       ///< 500kHz for low latency
    uint8_t coding_rate;      ///< 4/5 for minimal overhead
    int8_t tx_power;          ///< TX power in dBm
    char band_id[16];         ///< Hardware band ID (e.g., "HW_868")
    uint8_t aes_key[32];      ///< AES-256 encryption key
} lora_config_t;

/**
 * @brief Initialize LoRa driver
 *
 * Configures SX1262 with low-latency settings for presentation clicker.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lora_driver_init(void);

/**
 * @brief Send LoRa packet
 *
 * @param data Packet data to send
 * @param length Data length in bytes
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lora_send_packet(const uint8_t *data, size_t length);

/**
 * @brief Receive LoRa packet
 *
 * @param data Buffer to store received data
 * @param max_length Maximum buffer size
 * @param received_length Actual received length
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK on success, ESP_ERR_TIMEOUT on timeout
 */
esp_err_t lora_receive_packet(uint8_t *data, size_t max_length, size_t *received_length, uint32_t timeout_ms);

/**
 * @brief Get RSSI of last received packet
 *
 * @return RSSI in dBm
 */
int16_t lora_get_rssi(void);

/**
 * @brief Get current LoRa frequency
 *
 * @return Frequency in Hz
 */
uint32_t lora_get_frequency(void);

/**
 * @brief Get current LoRa configuration
 *
 * @param config Output LoRa configuration
 * @return ESP_OK on success
 */
esp_err_t lora_get_config(lora_config_t *config);

/**
 * @brief Load LoRa configuration from NVS
 *
 * @return ESP_OK on success
 */
esp_err_t lora_load_config_from_nvs(void);

/**
 * @brief Set LoRa configuration and save to NVS
 *
 * @param config New LoRa configuration
 * @return ESP_OK on success
 */
esp_err_t lora_set_config(const lora_config_t *config);

/**
 * @brief Set LoRa to receive mode
 *
 * @return ESP_OK on success
 */
esp_err_t lora_set_receive_mode(void);

#ifdef __cplusplus
}
#endif
