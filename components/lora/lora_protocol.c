/**
 * @file lora_protocol.c
 * @brief LoRaCue custom LoRa protocol implementation
 * 
 * CONTEXT: Secure packet structure with AES encryption and replay protection
 * PACKET: DeviceID(2) + Encrypted[SeqNum(2) + Cmd(1) + PayloadLen(1) + Payload(0-7)] + MAC(4)
 */

#include "lora_protocol.h"
#include "lora_driver.h"
#include "device_registry.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include <string.h>

static const char *TAG = "LORA_PROTOCOL";

// Protocol state
static bool protocol_initialized = false;
static uint16_t local_device_id = 0;
static uint16_t sequence_counter = 0;
static uint8_t aes_key[16];

// RSSI monitoring variables
static int16_t last_rssi = 0;
static uint64_t last_packet_time = 0;
static TaskHandle_t rssi_monitor_task_handle = NULL;
static bool rssi_monitor_running = false;
static mbedtls_aes_context aes_ctx;

esp_err_t lora_protocol_init(uint16_t device_id, const uint8_t *key)
{
    ESP_LOGI(TAG, "Initializing LoRa protocol for device 0x%04X", device_id);
    
    local_device_id = device_id;
    memcpy(aes_key, key, 16);
    
    // Initialize AES context
    mbedtls_aes_init(&aes_ctx);
    int ret = mbedtls_aes_setkey_enc(&aes_ctx, aes_key, 128);
    if (ret != 0) {
        ESP_LOGE(TAG, "AES key setup failed: %d", ret);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialize sequence counter with random value
    sequence_counter = esp_random() & 0xFFFF;
    
    protocol_initialized = true;
    ESP_LOGI(TAG, "LoRa protocol initialized successfully");
    
    return ESP_OK;
}

static esp_err_t calculate_mac(const uint8_t *data, size_t data_len, uint8_t *mac_out)
{
    // Simple MAC using first 4 bytes of HMAC-SHA256
    mbedtls_md_context_t md_ctx;
    mbedtls_md_init(&md_ctx);
    
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (mbedtls_md_setup(&md_ctx, md_info, 1) != 0) {
        mbedtls_md_free(&md_ctx);
        return ESP_FAIL;
    }
    
    if (mbedtls_md_hmac_starts(&md_ctx, aes_key, 16) != 0) {
        mbedtls_md_free(&md_ctx);
        return ESP_FAIL;
    }
    
    if (mbedtls_md_hmac_update(&md_ctx, data, data_len) != 0) {
        mbedtls_md_free(&md_ctx);
        return ESP_FAIL;
    }
    
    uint8_t full_mac[32];
    if (mbedtls_md_hmac_finish(&md_ctx, full_mac) != 0) {
        mbedtls_md_free(&md_ctx);
        return ESP_FAIL;
    }
    
    mbedtls_md_free(&md_ctx);
    
    // Use first 4 bytes as MAC
    memcpy(mac_out, full_mac, LORA_MAC_SIZE);
    return ESP_OK;
}

esp_err_t lora_protocol_send_command(lora_command_t command, const uint8_t *payload, uint8_t payload_length)
{
    if (!protocol_initialized) {
        ESP_LOGE(TAG, "Protocol not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (payload_length > LORA_PAYLOAD_MAX_SIZE) {
        ESP_LOGE(TAG, "Payload too large: %d bytes", payload_length);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Sending command 0x%02X with %d byte payload", command, payload_length);
    
    // Create packet data
    lora_packet_data_t packet_data = {
        .device_id = local_device_id,
        .sequence_num = ++sequence_counter,
        .command = command,
        .payload_length = payload_length,
    };
    
    if (payload && payload_length > 0) {
        memcpy(packet_data.payload, payload, payload_length);
    }
    
    // Create final packet
    lora_packet_t packet;
    packet.device_id = local_device_id;
    
    // Prepare data to encrypt (everything except device_id and mac)
    uint8_t plaintext[16] = {0}; // AES requires 16-byte blocks
    plaintext[0] = (packet_data.sequence_num >> 8) & 0xFF;
    plaintext[1] = packet_data.sequence_num & 0xFF;
    plaintext[2] = packet_data.command;
    plaintext[3] = packet_data.payload_length;
    if (payload_length > 0) {
        memcpy(&plaintext[4], packet_data.payload, payload_length);
    }
    // Remaining bytes are already zero-padded
    
    // Encrypt data (AES ECB with 16-byte blocks)
    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, plaintext, packet.encrypted_data);
    
    // Calculate MAC over device_id + encrypted_data
    uint8_t mac_data[18]; // 2 + 16 bytes
    mac_data[0] = (packet.device_id >> 8) & 0xFF;
    mac_data[1] = packet.device_id & 0xFF;
    memcpy(&mac_data[2], packet.encrypted_data, 16);
    
    esp_err_t ret = calculate_mac(mac_data, 18, packet.mac);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MAC calculation failed");
        return ret;
    }
    
    // Send packet
    ret = lora_send_packet((uint8_t*)&packet, sizeof(packet));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send packet: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Packet sent successfully (seq: %d)", packet_data.sequence_num);
    return ESP_OK;
}

esp_err_t lora_protocol_send_reliable(lora_command_t command, const uint8_t *payload, uint8_t payload_length, uint32_t timeout_ms, uint8_t max_retries)
{
    if (!protocol_initialized) {
        ESP_LOGE(TAG, "Protocol not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    uint16_t expected_ack_seq = sequence_counter + 1;
    
    for (uint8_t attempt = 0; attempt <= max_retries; attempt++) {
        // Send command
        esp_err_t ret = lora_protocol_send_command(command, payload, payload_length);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Send failed on attempt %d: %s", attempt + 1, esp_err_to_name(ret));
            continue;
        }
        
        // Wait for ACK
        uint32_t start_time = esp_timer_get_time() / 1000;
        while ((esp_timer_get_time() / 1000 - start_time) < timeout_ms) {
            lora_packet_data_t ack_packet;
            ret = lora_protocol_receive_packet(&ack_packet, 100);
            
            if (ret == ESP_OK && ack_packet.command == CMD_ACK && ack_packet.payload_length == 2) {
                uint16_t ack_seq = (ack_packet.payload[0] << 8) | ack_packet.payload[1];
                if (ack_seq == expected_ack_seq) {
                    ESP_LOGI(TAG, "ACK received for seq %d", expected_ack_seq);
                    return ESP_OK;
                }
            }
        }
        
        ESP_LOGW(TAG, "No ACK received, attempt %d/%d", attempt + 1, max_retries + 1);
    }
    
    ESP_LOGE(TAG, "Failed to get ACK after %d attempts", max_retries + 1);
    return ESP_ERR_TIMEOUT;
}

esp_err_t lora_protocol_receive_packet(lora_packet_data_t *packet_data, uint32_t timeout_ms)
{
    if (!protocol_initialized) {
        ESP_LOGE(TAG, "Protocol not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t rx_buffer[LORA_PACKET_MAX_SIZE];
    size_t received_length;
    
    esp_err_t ret = lora_receive_packet(rx_buffer, sizeof(rx_buffer), &received_length, timeout_ms);
    if (ret != ESP_OK) {
        return ret; // Timeout or error
    }
    
    if (received_length != sizeof(lora_packet_t)) {
        ESP_LOGW(TAG, "Invalid packet size: %d bytes", received_length);
        return ESP_ERR_INVALID_SIZE;
    }
    
    lora_packet_t *packet = (lora_packet_t*)rx_buffer;
    
    // Check if sender is paired
    paired_device_t sender_device;
    ret = device_registry_get(packet->device_id, &sender_device);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Packet from unknown device 0x%04X", packet->device_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    // Verify MAC using sender's key
    uint8_t mac_data[18]; // 2 + 16 bytes
    mac_data[0] = (packet->device_id >> 8) & 0xFF;
    mac_data[1] = packet->device_id & 0xFF;
    memcpy(&mac_data[2], packet->encrypted_data, 16);
    
    uint8_t calculated_mac[LORA_MAC_SIZE];
    
    // Temporarily use sender's key for MAC calculation
    uint8_t original_key[16];
    memcpy(original_key, aes_key, 16);
    memcpy(aes_key, sender_device.aes_key, 16);
    
    ret = calculate_mac(mac_data, 18, calculated_mac);
    
    // Restore original key
    memcpy(aes_key, original_key, 16);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MAC calculation failed");
        return ESP_FAIL;
    }
    
    if (memcmp(packet->mac, calculated_mac, LORA_MAC_SIZE) != 0) {
        ESP_LOGW(TAG, "MAC verification failed for device 0x%04X", packet->device_id);
        return ESP_ERR_INVALID_CRC;
    }
    
    // Decrypt data using sender's key
    uint8_t plaintext[16];
    mbedtls_aes_context sender_aes_ctx;
    mbedtls_aes_init(&sender_aes_ctx);
    mbedtls_aes_setkey_dec(&sender_aes_ctx, sender_device.aes_key, 128);
    mbedtls_aes_crypt_ecb(&sender_aes_ctx, MBEDTLS_AES_DECRYPT, packet->encrypted_data, plaintext);
    mbedtls_aes_free(&sender_aes_ctx);
    
    // Parse decrypted data
    packet_data->device_id = packet->device_id;
    packet_data->sequence_num = (plaintext[0] << 8) | plaintext[1];
    packet_data->command = plaintext[2];
    packet_data->payload_length = plaintext[3];
    
    if (packet_data->payload_length > LORA_PAYLOAD_MAX_SIZE) {
        ESP_LOGW(TAG, "Invalid payload length: %d", packet_data->payload_length);
        return ESP_ERR_INVALID_SIZE;
    }
    
    if (packet_data->payload_length > 0) {
        memcpy(packet_data->payload, &plaintext[4], packet_data->payload_length);
    }
    
    // Replay protection check using device registry
    if (packet_data->sequence_num <= sender_device.last_sequence) {
        ESP_LOGW(TAG, "Replay attack detected from 0x%04X: seq %d <= %d", 
                packet_data->device_id, packet_data->sequence_num, sender_device.last_sequence);
        return ESP_ERR_INVALID_STATE;
    }
    
    // Update device registry with new sequence number
    device_registry_update_last_seen(packet_data->device_id, packet_data->sequence_num);
    
    // Send ACK for non-ACK packets
    if (packet_data->command != CMD_ACK) {
        esp_err_t ack_ret = lora_protocol_send_ack(packet_data->device_id, packet_data->sequence_num);
        if (ack_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send ACK: %s", esp_err_to_name(ack_ret));
        }
    }
    
    // Update RSSI and timestamp for connection monitoring
    last_rssi = lora_get_rssi();
    last_packet_time = esp_timer_get_time();
    
    ESP_LOGI(TAG, "Valid packet from %s (0x%04X): cmd=0x%02X, seq=%d, RSSI=%d dBm", 
             sender_device.device_name, packet_data->device_id, 
             packet_data->command, packet_data->sequence_num, last_rssi);
    
    return ESP_OK;
}

esp_err_t lora_protocol_send_ack(uint16_t to_device_id, uint16_t ack_sequence_num)
{
    uint8_t ack_payload[2] = {
        (ack_sequence_num >> 8) & 0xFF,
        ack_sequence_num & 0xFF
    };
    
    return lora_protocol_send_command(CMD_ACK, ack_payload, 2);
}

uint16_t lora_protocol_get_next_sequence(void)
{
    return sequence_counter + 1;
}

lora_connection_state_t lora_protocol_get_connection_state(void)
{
    uint64_t now = esp_timer_get_time();
    uint64_t time_since_last_packet = now - last_packet_time;
    
    // Connection lost if no packets for 30 seconds
    if (time_since_last_packet > 30000000) {
        return LORA_CONNECTION_LOST;
    }
    
    // Evaluate based on RSSI
    if (last_rssi > -70) {
        return LORA_CONNECTION_EXCELLENT;
    } else if (last_rssi > -85) {
        return LORA_CONNECTION_GOOD;
    } else if (last_rssi > -100) {
        return LORA_CONNECTION_WEAK;
    } else {
        return LORA_CONNECTION_POOR;
    }
}

int16_t lora_protocol_get_last_rssi(void)
{
    return last_rssi;
}

static void rssi_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "RSSI monitor task started");
    
    while (rssi_monitor_running) {
        lora_connection_state_t state = lora_protocol_get_connection_state();
        
        // Log connection quality changes
        static lora_connection_state_t last_state = LORA_CONNECTION_LOST;
        if (state != last_state) {
            const char* state_names[] = {"EXCELLENT", "GOOD", "WEAK", "POOR", "LOST"};
            ESP_LOGI(TAG, "Connection state: %s (RSSI: %d dBm)", 
                     state_names[state], last_rssi);
            last_state = state;
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
    
    ESP_LOGI(TAG, "RSSI monitor task stopped");
    vTaskDelete(NULL);
}

esp_err_t lora_protocol_start_rssi_monitor(void)
{
    if (rssi_monitor_running) {
        return ESP_OK;
    }
    
    rssi_monitor_running = true;
    BaseType_t ret = xTaskCreate(
        rssi_monitor_task,
        "lora_rssi",
        2048,
        NULL,
        3,
        &rssi_monitor_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RSSI monitor task");
        rssi_monitor_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "RSSI monitor started");
    return ESP_OK;
}
