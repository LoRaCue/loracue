/**
 * @file usb_pairing.h
 * @brief USB-C pairing protocol for secure device registration
 * 
 * CONTEXT: Ticket 5.1 - USB-C Pairing Protocol
 * PURPOSE: Secure key exchange between STAGE and PC over USB CDC
 * PROTOCOL: Challenge-response with AES key generation
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_PAIRING_TIMEOUT_MS      30000   ///< 30 second pairing timeout
#define USB_PAIRING_CHALLENGE_SIZE  16      ///< Challenge size in bytes
#define USB_PAIRING_KEY_SIZE        16      ///< AES key size in bytes
#define USB_PAIRING_DEVICE_NAME_MAX 32      ///< Maximum device name length

/**
 * @brief USB pairing states
 */
typedef enum {
    USB_PAIRING_IDLE,           ///< Not in pairing mode
    USB_PAIRING_WAITING,        ///< Waiting for PC connection
    USB_PAIRING_CONNECTED,      ///< USB connected, ready to pair
    USB_PAIRING_CHALLENGE,      ///< Exchanging challenge-response
    USB_PAIRING_KEY_EXCHANGE,   ///< Exchanging encryption keys
    USB_PAIRING_SUCCESS,        ///< Pairing completed successfully
    USB_PAIRING_FAILED,         ///< Pairing failed
    USB_PAIRING_TIMEOUT,        ///< Pairing timed out
} usb_pairing_state_t;

/**
 * @brief Pairing message types
 */
typedef enum {
    PAIRING_MSG_HELLO = 0x01,       ///< Initial handshake
    PAIRING_MSG_CHALLENGE = 0x02,   ///< Challenge request
    PAIRING_MSG_RESPONSE = 0x03,    ///< Challenge response
    PAIRING_MSG_KEY_OFFER = 0x04,   ///< AES key offer
    PAIRING_MSG_KEY_ACCEPT = 0x05,  ///< Key acceptance
    PAIRING_MSG_COMPLETE = 0x06,    ///< Pairing complete
    PAIRING_MSG_ERROR = 0xFF,       ///< Error message
} pairing_msg_type_t;

/**
 * @brief Pairing message structure
 */
typedef struct __attribute__((packed)) {
    uint8_t type;                           ///< Message type
    uint8_t length;                         ///< Payload length
    uint8_t payload[62];                    ///< Message payload (max 62 bytes for 64-byte packets)
} pairing_message_t;

/**
 * @brief Device information for pairing
 */
typedef struct {
    uint16_t device_id;                     ///< Device ID
    char device_name[USB_PAIRING_DEVICE_NAME_MAX]; ///< Device name
    uint8_t mac_address[6];                 ///< MAC address
    uint8_t firmware_version[4];            ///< Firmware version
} device_info_t;

/**
 * @brief Pairing result callback
 */
typedef void (*pairing_result_callback_t)(bool success, uint16_t device_id, const char* device_name);

/**
 * @brief Initialize USB pairing system
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_pairing_init(void);

/**
 * @brief Start pairing mode (STAGE device)
 * 
 * @param device_name This device's name
 * @param callback Callback for pairing result
 * @return ESP_OK on success
 */
esp_err_t usb_pairing_start_stage(const char* device_name, pairing_result_callback_t callback);

/**
 * @brief Start pairing mode (PC receiver)
 * 
 * @param callback Callback for pairing result
 * @return ESP_OK on success
 */
esp_err_t usb_pairing_start_pc(pairing_result_callback_t callback);

/**
 * @brief Stop pairing mode
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_pairing_stop(void);

/**
 * @brief Get current pairing state
 * 
 * @return Current pairing state
 */
usb_pairing_state_t usb_pairing_get_state(void);

/**
 * @brief Check if USB is connected
 * 
 * @return true if USB is connected
 */
bool usb_pairing_is_connected(void);

/**
 * @brief Process pairing (call regularly in main loop)
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_pairing_process(void);

/**
 * @brief Generate secure pairing PIN for display
 * 
 * @param pin_buffer Buffer to store 6-digit PIN
 * @param buffer_size Buffer size (minimum 7 bytes for null terminator)
 * @return ESP_OK on success
 */
esp_err_t usb_pairing_generate_pin(char* pin_buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif
