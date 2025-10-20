/**
 * @file lora_protocol.h
 * @brief LoRaCue LoRa Protocol
 *
 * PACKET: DeviceID(2) + Encrypted[SeqNum(2) + Cmd(1) + Payload(7)] + MAC(4)
 * Uses CMD_HID_REPORT with structured payload for extensible HID support
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LORA_PACKET_MAX_SIZE 22
#define LORA_DEVICE_ID_SIZE 2
#define LORA_SEQUENCE_NUM_SIZE 2
#define LORA_COMMAND_SIZE 1
#define LORA_PAYLOAD_MAX_SIZE 7
#define LORA_MAC_SIZE 4

// Protocol macros
#define LORA_PROTOCOL_VERSION 0x01
#define LORA_DEFAULT_SLOT 1

// Byte 0: version_slot
#define LORA_VERSION(vs) (((vs) >> 4) & 0x0F)
#define LORA_SLOT(vs) ((vs) & 0x0F)
#define LORA_MAKE_VS(v, s) ((((v) & 0x0F) << 4) | ((s) & 0x0F))

// Byte 1: type_flags
#define LORA_HID_TYPE(tf) (((tf) >> 4) & 0x0F)
#define LORA_FLAGS(tf) ((tf) & 0x0F)
#define LORA_MAKE_TF(t, f) ((((t) & 0x0F) << 4) | ((f) & 0x0F))

/**
 * @brief LoRa command types
 */
typedef enum {
    CMD_HID_REPORT = 0x01, ///< HID report with structured payload
    CMD_ACK        = 0xAC, ///< Acknowledgment (AC = ACk)
} lora_command_t;

/**
 * @brief LoRa RX callback
 * @param device_id Sender device ID
 * @param command Received command
 * @param payload Payload data
 * @param payload_length Payload length
 * @param rssi Signal strength in dBm
 * @param user_ctx User context
 */
typedef void (*lora_protocol_rx_callback_t)(uint16_t device_id, lora_command_t command, const uint8_t *payload,
                                            uint8_t payload_length, int16_t rssi, void *user_ctx);

/**
 * @brief HID device types
 */
typedef enum {
    HID_TYPE_NONE     = 0x0, ///< No HID device
    HID_TYPE_KEYBOARD = 0x1, ///< Keyboard
    HID_TYPE_MOUSE    = 0x2, ///< Mouse
    HID_TYPE_MEDIA    = 0x3, ///< Media keys
} lora_hid_type_t;

/**
 * @brief Keyboard HID report (5 bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t modifiers;  ///< Bit 0=Ctrl, 1=Shift, 2=Alt, 3=GUI
    uint8_t keycode[4]; ///< Up to 4 simultaneous keys
} lora_keyboard_report_t;

/**
 * @brief Payload structure (7 bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t version_slot; ///< [7:4]=protocol_ver, [3:0]=slot_id (1-16)
    uint8_t type_flags;   ///< [7:4]=hid_type, [3:0]=flags/reserved
    union {
        uint8_t raw[5];
        lora_keyboard_report_t keyboard;
    } hid_report;
} lora_payload_v2_t;

/**
 * @brief LoRa packet structure (before encryption)
 */
typedef struct __attribute__((packed)) {
    uint16_t device_id;                     ///< Unique device identifier
    uint16_t sequence_num;                  ///< Sequence number for replay protection
    uint8_t command;                        ///< Command type
    uint8_t payload_length;                 ///< Payload length (0-7)
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
 */
esp_err_t lora_protocol_init(uint16_t device_id, const uint8_t *aes_key);

/**
 * @brief Send keyboard key press
 */
esp_err_t lora_protocol_send_keyboard(uint8_t slot_id, uint8_t modifiers, uint8_t keycode);

/**
 * @brief Send keyboard with ACK
 */
esp_err_t lora_protocol_send_keyboard_reliable(uint8_t slot_id, uint8_t modifiers, uint8_t keycode, uint32_t timeout_ms,
                                               uint8_t max_retries);

/**
 * @brief Receive and decrypt LoRa packet
 */
esp_err_t lora_protocol_receive_packet(lora_packet_data_t *packet_data, uint32_t timeout_ms);

/**
 * @brief Send command with ACK and retries
 */
esp_err_t lora_protocol_send_reliable(lora_command_t command, const uint8_t *payload, uint8_t payload_length,
                                      uint32_t timeout_ms, uint8_t max_retries);

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
    LORA_CONNECTION_EXCELLENT = 0, ///< RSSI > -70 dBm
    LORA_CONNECTION_GOOD      = 1, ///< RSSI > -85 dBm
    LORA_CONNECTION_WEAK      = 2, ///< RSSI > -100 dBm
    LORA_CONNECTION_POOR      = 3, ///< RSSI <= -100 dBm
    LORA_CONNECTION_LOST      = 4  ///< No packets received recently
} lora_connection_state_t;

/**
 * @brief LoRa connection state callback
 * @param state New connection state
 * @param user_ctx User context
 */
typedef void (*lora_protocol_state_callback_t)(lora_connection_state_t state, void *user_ctx);

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

/**
 * @brief Register RX callback
 * @param callback Callback function
 * @param user_ctx User context
 */
void lora_protocol_register_rx_callback(lora_protocol_rx_callback_t callback, void *user_ctx);

/**
 * @brief Register state change callback
 * @param callback Callback function
 * @param user_ctx User context
 */
void lora_protocol_register_state_callback(lora_protocol_state_callback_t callback, void *user_ctx);

/**
 * @brief Start protocol RX task
 * @return ESP_OK on success
 */
esp_err_t lora_protocol_start(void);

#ifdef __cplusplus
}
#endif
