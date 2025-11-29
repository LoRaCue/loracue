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

// LoRa network configuration
#define LORA_PRIVATE_SYNC_WORD  0x1424  ///< Private network sync word (prevents public network interference)

/**
 * @brief LoRa bandwidth options (kHz)
 */
typedef enum {
    LORA_BW_7_8KHZ   = 7,    ///< 7.8 kHz
    LORA_BW_10_4KHZ  = 10,   ///< 10.4 kHz
    LORA_BW_15_6KHZ  = 15,   ///< 15.6 kHz
    LORA_BW_20_8KHZ  = 20,   ///< 20.8 kHz
    LORA_BW_31_25KHZ = 31,   ///< 31.25 kHz
    LORA_BW_41_7KHZ  = 41,   ///< 41.7 kHz
    LORA_BW_62_5KHZ  = 62,   ///< 62.5 kHz
    LORA_BW_125KHZ   = 125,  ///< 125 kHz
    LORA_BW_250KHZ   = 250,  ///< 250 kHz
    LORA_BW_500KHZ   = 500   ///< 500 kHz (low latency)
} lora_bandwidth_t;

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
