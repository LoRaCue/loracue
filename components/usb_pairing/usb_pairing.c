/**
 * @file usb_pairing.c
 * @brief USB-C pairing protocol implementation
 * 
 * CONTEXT: Secure device pairing over USB CDC with challenge-response
 * PROTOCOL: HELLO → CHALLENGE → RESPONSE → KEY_EXCHANGE → COMPLETE
 */

#include "usb_pairing.h"
#include "device_registry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "mbedtls/sha256.h"
#include "mbedtls/aes.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "USB_PAIRING";

// Pairing state
static bool pairing_initialized = false;
static usb_pairing_state_t current_state = USB_PAIRING_IDLE;
static pairing_result_callback_t result_callback = NULL;
static uint64_t pairing_start_time = 0;
static bool is_stage_device = false;

// Pairing data
static device_info_t local_device_info = {0};
static device_info_t remote_device_info = {0};
static uint8_t challenge[USB_PAIRING_CHALLENGE_SIZE];
static uint8_t response[USB_PAIRING_CHALLENGE_SIZE];
static uint8_t shared_key[USB_PAIRING_KEY_SIZE];
static char pairing_pin[7] = {0}; // 6 digits + null terminator

// Message buffer
static uint8_t tx_buffer[64];
static uint8_t rx_buffer[64];

static esp_err_t send_pairing_message(const pairing_message_t* msg)
{
    // Placeholder for USB CDC transmission
    ESP_LOGI(TAG, "📤 Sending message type 0x%02X, length %d", msg->type, msg->length);
    
    // In real implementation, this would send via USB CDC
    // For now, just log the message
    
    return ESP_OK;
}

static esp_err_t receive_pairing_message(pairing_message_t* msg, uint32_t timeout_ms)
{
    // Placeholder for USB CDC reception
    ESP_LOGD(TAG, "📥 Waiting for message (timeout: %dms)", timeout_ms);
    
    // In real implementation, this would receive via USB CDC
    // For now, simulate timeout
    
    return ESP_ERR_TIMEOUT;
}

static void generate_challenge(uint8_t* challenge_buf)
{
    for (int i = 0; i < USB_PAIRING_CHALLENGE_SIZE; i++) {
        challenge_buf[i] = esp_random() & 0xFF;
    }
}

static esp_err_t compute_response(const uint8_t* challenge_buf, const char* pin, uint8_t* response_buf)
{
    // Compute SHA256(challenge + PIN)
    mbedtls_sha256_context sha_ctx;
    mbedtls_sha256_init(&sha_ctx);
    
    esp_err_t ret = ESP_OK;
    
    if (mbedtls_sha256_starts(&sha_ctx, 0) != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    
    if (mbedtls_sha256_update(&sha_ctx, challenge_buf, USB_PAIRING_CHALLENGE_SIZE) != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    
    if (mbedtls_sha256_update(&sha_ctx, (const uint8_t*)pin, strlen(pin)) != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    
    uint8_t hash[32];
    if (mbedtls_sha256_finish(&sha_ctx, hash) != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    
    // Use first 16 bytes as response
    memcpy(response_buf, hash, USB_PAIRING_CHALLENGE_SIZE);
    
cleanup:
    mbedtls_sha256_free(&sha_ctx);
    return ret;
}

static esp_err_t generate_shared_key(const uint8_t* challenge_buf, const uint8_t* response_buf, uint8_t* key_buf)
{
    // Generate shared key from challenge and response
    mbedtls_sha256_context sha_ctx;
    mbedtls_sha256_init(&sha_ctx);
    
    esp_err_t ret = ESP_OK;
    
    if (mbedtls_sha256_starts(&sha_ctx, 0) != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    
    if (mbedtls_sha256_update(&sha_ctx, challenge_buf, USB_PAIRING_CHALLENGE_SIZE) != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    
    if (mbedtls_sha256_update(&sha_ctx, response_buf, USB_PAIRING_CHALLENGE_SIZE) != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    
    uint8_t hash[32];
    if (mbedtls_sha256_finish(&sha_ctx, hash) != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    
    // Use first 16 bytes as AES key
    memcpy(key_buf, hash, USB_PAIRING_KEY_SIZE);
    
cleanup:
    mbedtls_sha256_free(&sha_ctx);
    return ret;
}

esp_err_t usb_pairing_init(void)
{
    ESP_LOGI(TAG, "Initializing USB pairing system");
    
    // Initialize local device info
    local_device_info.device_id = esp_random() & 0xFFFF;
    strcpy(local_device_info.device_name, "LoRaCue-Device");
    
    // Get MAC address (placeholder)
    for (int i = 0; i < 6; i++) {
        local_device_info.mac_address[i] = esp_random() & 0xFF;
    }
    
    // Set firmware version
    local_device_info.firmware_version[0] = 1; // Major
    local_device_info.firmware_version[1] = 0; // Minor
    local_device_info.firmware_version[2] = 0; // Patch
    local_device_info.firmware_version[3] = 0; // Build
    
    pairing_initialized = true;
    ESP_LOGI(TAG, "USB pairing initialized (Device ID: 0x%04X)", local_device_info.device_id);
    
    return ESP_OK;
}

esp_err_t usb_pairing_start_stage(const char* device_name, pairing_result_callback_t callback)
{
    if (!pairing_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting STAGE pairing mode");
    
    // Update device name
    strncpy(local_device_info.device_name, device_name, USB_PAIRING_DEVICE_NAME_MAX - 1);
    local_device_info.device_name[USB_PAIRING_DEVICE_NAME_MAX - 1] = '\0';
    
    current_state = USB_PAIRING_WAITING;
    result_callback = callback;
    pairing_start_time = esp_timer_get_time();
    is_stage_device = true;
    
    // Generate pairing PIN
    usb_pairing_generate_pin(pairing_pin, sizeof(pairing_pin));
    
    ESP_LOGI(TAG, "STAGE pairing started - PIN: %s", pairing_pin);
    ESP_LOGI(TAG, "Connect USB-C cable to PC and enter PIN on PC");
    
    return ESP_OK;
}

esp_err_t usb_pairing_start_pc(pairing_result_callback_t callback)
{
    if (!pairing_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting PC pairing mode");
    
    current_state = USB_PAIRING_WAITING;
    result_callback = callback;
    pairing_start_time = esp_timer_get_time();
    is_stage_device = false;
    
    ESP_LOGI(TAG, "PC pairing started - waiting for STAGE device connection");
    
    return ESP_OK;
}

esp_err_t usb_pairing_stop(void)
{
    ESP_LOGI(TAG, "Stopping pairing mode");
    
    current_state = USB_PAIRING_IDLE;
    result_callback = NULL;
    pairing_start_time = 0;
    
    // Clear sensitive data
    memset(challenge, 0, sizeof(challenge));
    memset(response, 0, sizeof(response));
    memset(shared_key, 0, sizeof(shared_key));
    memset(pairing_pin, 0, sizeof(pairing_pin));
    
    return ESP_OK;
}

usb_pairing_state_t usb_pairing_get_state(void)
{
    return current_state;
}

bool usb_pairing_is_connected(void)
{
    // Placeholder - would check actual USB connection status
    return (current_state >= USB_PAIRING_CONNECTED);
}

esp_err_t usb_pairing_process(void)
{
    if (current_state == USB_PAIRING_IDLE) {
        return ESP_OK;
    }
    
    // Check for timeout
    uint64_t current_time = esp_timer_get_time();
    if ((current_time - pairing_start_time) > (USB_PAIRING_TIMEOUT_MS * 1000ULL)) {
        ESP_LOGW(TAG, "Pairing timeout");
        current_state = USB_PAIRING_TIMEOUT;
        
        if (result_callback) {
            result_callback(false, 0, "Timeout");
        }
        
        usb_pairing_stop();
        return ESP_ERR_TIMEOUT;
    }
    
    switch (current_state) {
        case USB_PAIRING_WAITING:
            // Check for USB connection
            // In real implementation, this would check USB CDC status
            // For demo, simulate connection after 2 seconds
            if ((current_time - pairing_start_time) > 2000000ULL) { // 2 seconds
                current_state = USB_PAIRING_CONNECTED;
                ESP_LOGI(TAG, "USB connected, starting handshake");
            }
            break;
            
        case USB_PAIRING_CONNECTED:
            // Start pairing handshake
            if (is_stage_device) {
                // STAGE sends HELLO message
                pairing_message_t hello_msg = {
                    .type = PAIRING_MSG_HELLO,
                    .length = sizeof(device_info_t)
                };
                memcpy(hello_msg.payload, &local_device_info, sizeof(device_info_t));
                
                send_pairing_message(&hello_msg);
                current_state = USB_PAIRING_CHALLENGE;
                ESP_LOGI(TAG, "Sent HELLO message");
            } else {
                // PC waits for HELLO
                pairing_message_t msg;
                if (receive_pairing_message(&msg, 100) == ESP_OK) {
                    if (msg.type == PAIRING_MSG_HELLO) {
                        memcpy(&remote_device_info, msg.payload, sizeof(device_info_t));
                        current_state = USB_PAIRING_CHALLENGE;
                        ESP_LOGI(TAG, "Received HELLO from %s", remote_device_info.device_name);
                    }
                }
            }
            break;
            
        case USB_PAIRING_CHALLENGE:
            // Simulate successful pairing for demo
            ESP_LOGI(TAG, "Simulating successful pairing...");
            
            // Generate dummy shared key
            for (int i = 0; i < USB_PAIRING_KEY_SIZE; i++) {
                shared_key[i] = esp_random() & 0xFF;
            }
            
            // Add device to registry
            if (is_stage_device) {
                // STAGE device doesn't add itself, PC does the adding
                current_state = USB_PAIRING_SUCCESS;
            } else {
                // PC adds the STAGE device
                esp_err_t ret = device_registry_add(
                    remote_device_info.device_id,
                    remote_device_info.device_name,
                    remote_device_info.mac_address,
                    shared_key
                );
                
                if (ret == ESP_OK) {
                    current_state = USB_PAIRING_SUCCESS;
                    ESP_LOGI(TAG, "Device %s paired successfully", remote_device_info.device_name);
                } else {
                    current_state = USB_PAIRING_FAILED;
                    ESP_LOGE(TAG, "Failed to add device to registry");
                }
            }
            break;
            
        case USB_PAIRING_SUCCESS:
            ESP_LOGI(TAG, "✓ Pairing completed successfully");
            
            if (result_callback) {
                if (is_stage_device) {
                    result_callback(true, local_device_info.device_id, local_device_info.device_name);
                } else {
                    result_callback(true, remote_device_info.device_id, remote_device_info.device_name);
                }
            }
            
            usb_pairing_stop();
            break;
            
        case USB_PAIRING_FAILED:
            ESP_LOGE(TAG, "✗ Pairing failed");
            
            if (result_callback) {
                result_callback(false, 0, "Pairing failed");
            }
            
            usb_pairing_stop();
            break;
            
        default:
            break;
    }
    
    return ESP_OK;
}

esp_err_t usb_pairing_generate_pin(char* pin_buffer, size_t buffer_size)
{
    if (!pin_buffer || buffer_size < 7) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Generate 6-digit PIN
    uint32_t pin_num = esp_random() % 1000000;
    snprintf(pin_buffer, buffer_size, "%06lu", (unsigned long)pin_num);
    
    return ESP_OK;
}
