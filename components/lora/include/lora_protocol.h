/**
 * @file lora_protocol.h
 * @brief LoRaCue custom LoRa protocol implementation
 * 
 * CONTEXT: Ticket 2.2 - Custom Protocol Implementation
 * PACKET: DeviceID(2) + SequenceNum(2) + Command(1) + Payload(0-7) + MAC(4)
 * SECURITY: AES-128 encryption with replay protection
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LORA_PACKET_MAX_SIZE        22  // 2 + 16 + 4 bytes
#define LORA_DEVICE_ID_SIZE         2
#define LORA_SEQUENCE_NUM_SIZE      2
#define LORA_COMMAND_SIZE           1
#define LORA_PAYLOAD_MAX_SIZE       7
#define LORA_MAC_SIZE               4

/**
 * @brief LoRaCue command types
 */
typedef enum {
    CMD_NEXT_SLIDE = 0x01,      ///< Next slide command
    CMD_PREV_SLIDE = 0x02,      ///< Previous slide command
    CMD_BLACK_SCREEN = 0x03,    ///< Black screen toggle
    CMD_START_PRESENTATION = 0x04, ///< Start presentation (F5)
    CMD_ACK = 0x80,             ///< Acknowledgment
} lora_command_t;

/**
 * @brief LoRa packet structure (before encryption)
 */
typedef struct __attribute__((packed)) {
    uint16_t device_id;         ///< Unique device identifier
    uint16_t sequence_num;      ///< Sequence number for replay protection
    uint8_t command;            ///< Command type
    uint8_t payload_length;     ///< Payload length (0-7)
    uint8_t payload[LORA_PAYLOAD_MAX_SIZE]; ///< Variable payload data
} lora_packet_data_t;

/**
 * @brief Complete LoRa packet (with MAC)
 */
typedef struct __attribute__((packed)) {
    uint16_t device_id;         ///< Device ID (unencrypted)
    uint8_t encrypted_data[16]; ///< Encrypted: seq_num + cmd + payload_len + payload (padded to 16 bytes)
    uint8_t mac[LORA_MAC_SIZE]; ///< Message Authentication Code
} lora_packet_t;

/**
 * @brief Initialize LoRa protocol
 * 
 * @param device_id This device's unique ID
 * @param aes_key 16-byte AES key for encryption
 * @return ESP_OK on success
 */
esp_err_t lora_protocol_init(uint16_t device_id, const uint8_t *aes_key);

/**
 * @brief Create and send LoRa packet
 * 
 * @param command Command to send
 * @param payload Optional payload data
 * @param payload_length Payload length (0-7)
 * @return ESP_OK on success
 */
esp_err_t lora_protocol_send_command(lora_command_t command, const uint8_t *payload, uint8_t payload_length);

/**
 * @brief Send command with ACK and retransmission
 * 
 * @param command Command to send
 * @param payload Optional payload data
 * @param payload_length Payload length (0-7)
 * @param timeout_ms Timeout for ACK reception
 * @param max_retries Maximum retry attempts
 * @return ESP_OK on success, ESP_ERR_TIMEOUT if no ACK received
 */
esp_err_t lora_protocol_send_reliable(lora_command_t command, const uint8_t *payload, uint8_t payload_length, uint32_t timeout_ms, uint8_t max_retries);

/**
 * @brief Receive and validate LoRa packet
 * 
 * @param packet_data Decoded packet data
 * @param timeout_ms Receive timeout in milliseconds
 * @return ESP_OK on success, ESP_ERR_TIMEOUT on timeout
 */
esp_err_t lora_protocol_receive_packet(lora_packet_data_t *packet_data, uint32_t timeout_ms);

/**
 * @brief Send ACK packet
 * 
 * @param to_device_id Device ID to send ACK to
 * @param ack_sequence_num Sequence number being acknowledged
 * @return ESP_OK on success
 */
esp_err_t lora_protocol_send_ack(uint16_t to_device_id, uint16_t ack_sequence_num);

/**
 * @brief Get next sequence number
 * 
 * @return Next sequence number to use
 */
uint16_t lora_protocol_get_next_sequence(void);

/**
 * @brief LoRa connection quality states
 */
typedef enum {
    LORA_CONNECTION_EXCELLENT = 0,  ///< RSSI > -70 dBm
    LORA_CONNECTION_GOOD = 1,       ///< RSSI > -85 dBm  
    LORA_CONNECTION_WEAK = 2,       ///< RSSI > -100 dBm
    LORA_CONNECTION_POOR = 3,       ///< RSSI <= -100 dBm
    LORA_CONNECTION_LOST = 4        ///< No packets received recently
} lora_connection_state_t;

/**
 * @brief Get current connection quality
 * 
 * @return Current connection state based on RSSI and activity
 */
lora_connection_state_t lora_protocol_get_connection_state(void);

/**
 * @brief Get last RSSI value
 * 
 * @return RSSI in dBm, or 0 if no recent packets
 */
int16_t lora_protocol_get_last_rssi(void);

/**
 * @brief Start RSSI monitoring task
 * 
 * @return ESP_OK on success
 */
esp_err_t lora_protocol_start_rssi_monitor(void);

/**
 * @brief Connection statistics
 */
typedef struct {
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t acks_received;
    uint32_t retransmissions;
    uint32_t failed_transmissions;
    float packet_loss_rate;
} lora_connection_stats_t;

/**
 * @brief Get connection statistics
 * 
 * @param stats Pointer to store statistics
 * @return ESP_OK on success
 */
esp_err_t lora_protocol_get_stats(lora_connection_stats_t *stats);

/**
 * @brief Reset connection statistics
 */
void lora_protocol_reset_stats(void);

#ifdef __cplusplus
}
#endif
