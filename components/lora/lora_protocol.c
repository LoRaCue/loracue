/**
 * @file lora_protocol.c
 * @brief LoRaCue custom LoRa protocol implementation
 *
 * CONTEXT: Secure packet structure with AES encryption and replay protection
 * PACKET: DeviceID(2) + Encrypted[SeqNum(2) + Cmd(1) + PayloadLen(1) + Payload(0-7)] + MAC(4)
 */

#include "lora_protocol.h"
#include "device_registry.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora_driver.h"
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include <string.h>

static const char *TAG = "LORA_PROTOCOL";

// Protocol state
static bool protocol_initialized = false;
static uint16_t local_device_id  = 0;
static uint16_t sequence_counter = 0;
static uint8_t local_device_key[32]; // Local device's AES-256 key

// RSSI monitoring variables
static int16_t last_rssi                     = 0;
static uint64_t last_packet_time             = 0;
static TaskHandle_t rssi_monitor_task_handle = NULL;
static bool rssi_monitor_running             = false;

// Connection statistics
static lora_connection_stats_t connection_stats = {0};
static mbedtls_aes_context aes_ctx;

esp_err_t lora_protocol_init(uint16_t device_id, const uint8_t *key)
{
    ESP_LOGI(TAG, "Initializing LoRa protocol for device 0x%04X with AES-256", device_id);

    local_device_id = device_id;
    memcpy(local_device_key, key, 32);

    // Initialize AES context
    mbedtls_aes_init(&aes_ctx);
    int ret = mbedtls_aes_setkey_enc(&aes_ctx, local_device_key, 256);
    if (ret != 0) {
        ESP_LOGE(TAG, "AES-256 key setup failed: %d", ret);
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize sequence counter with random value
    sequence_counter = esp_random() & 0xFFFF;

    protocol_initialized = true;
    ESP_LOGI(TAG, "LoRa protocol initialized with AES-256 encryption");

    return ESP_OK;
}

static esp_err_t calculate_mac(const uint8_t *data, size_t data_len, const uint8_t *key, uint8_t *mac_out)
{
    // MAC using first 4 bytes of HMAC-SHA256 with AES-256 key
    mbedtls_md_context_t md_ctx;
    mbedtls_md_init(&md_ctx);

    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (mbedtls_md_setup(&md_ctx, md_info, 1) != 0) {
        mbedtls_md_free(&md_ctx);
        return ESP_FAIL;
    }

    if (mbedtls_md_hmac_starts(&md_ctx, key, 32) != 0) {
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

static esp_err_t lora_protocol_send_command(lora_command_t command, const uint8_t *payload, uint8_t payload_length)
{
    lora_packet_t packet;
    packet.device_id = local_device_id;

    uint8_t plaintext[16] = {0};
    plaintext[0]          = (sequence_counter >> 8) & 0xFF;
    plaintext[1]          = sequence_counter & 0xFF;
    plaintext[2]          = command;
    plaintext[3]          = payload_length;
    if (payload && payload_length > 0) {
        memcpy(&plaintext[4], payload, payload_length);
    }

    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, plaintext, packet.encrypted_data);

    uint8_t mac_data[18];
    memcpy(mac_data, &packet.device_id, 2);
    memcpy(&mac_data[2], packet.encrypted_data, 16);
    calculate_mac(mac_data, 18, local_device_key, packet.mac);

    sequence_counter++;
    connection_stats.packets_sent++;

    return lora_send_packet((uint8_t *)&packet, sizeof(packet));
}

esp_err_t lora_protocol_send_keyboard(uint8_t slot_id, uint8_t modifiers, uint8_t keycode)
{
    if (!protocol_initialized) {
        ESP_LOGE(TAG, "Protocol not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Sending keyboard: slot=%d mod=0x%02X key=0x%02X", slot_id, modifiers, keycode);

    lora_payload_v2_t payload_v2;
    payload_v2.version_slot                   = LORA_MAKE_VS(LORA_PROTOCOL_VERSION, slot_id);
    payload_v2.type_flags                     = LORA_MAKE_TF(HID_TYPE_KEYBOARD, 0);
    payload_v2.hid_report.keyboard.modifiers  = modifiers;
    payload_v2.hid_report.keyboard.keycode[0] = keycode;
    payload_v2.hid_report.keyboard.keycode[1] = 0;
    payload_v2.hid_report.keyboard.keycode[2] = 0;
    payload_v2.hid_report.keyboard.keycode[3] = 0;

    return lora_protocol_send_command(CMD_HID_REPORT, (const uint8_t *)&payload_v2, sizeof(lora_payload_v2_t));
}

esp_err_t lora_protocol_send_keyboard_reliable(uint8_t slot_id, uint8_t modifiers, uint8_t keycode, uint32_t timeout_ms,
                                               uint8_t max_retries)
{
    if (!protocol_initialized) {
        ESP_LOGE(TAG, "Protocol not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    lora_payload_v2_t payload_v2;
    payload_v2.version_slot                   = LORA_MAKE_VS(LORA_PROTOCOL_VERSION, slot_id);
    payload_v2.type_flags                     = LORA_MAKE_TF(HID_TYPE_KEYBOARD, 0);
    payload_v2.hid_report.keyboard.modifiers  = modifiers;
    payload_v2.hid_report.keyboard.keycode[0] = keycode;
    payload_v2.hid_report.keyboard.keycode[1] = 0;
    payload_v2.hid_report.keyboard.keycode[2] = 0;
    payload_v2.hid_report.keyboard.keycode[3] = 0;

    return lora_protocol_send_reliable(CMD_HID_REPORT, (const uint8_t *)&payload_v2, sizeof(lora_payload_v2_t),
                                       timeout_ms, max_retries);
}

esp_err_t lora_protocol_send_reliable(lora_command_t command, const uint8_t *payload, uint8_t payload_length,
                                      uint32_t timeout_ms, uint8_t max_retries)
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
            if (attempt > 0)
                connection_stats.retransmissions++;
            continue;
        }

        // Track retransmissions
        if (attempt > 0) {
            connection_stats.retransmissions++;
        }

        // Wait for ACK
        uint32_t start_time = esp_timer_get_time() / 1000;
        while ((esp_timer_get_time() / 1000 - start_time) < timeout_ms) {
            lora_packet_data_t ack_packet;
            ret = lora_protocol_receive_packet(&ack_packet, 100);

            if (ret == ESP_OK && ack_packet.command == CMD_ACK && ack_packet.payload_length == 2) {
                uint16_t ack_seq = (ack_packet.payload[0] << 8) | ack_packet.payload[1];
                if (ack_seq == expected_ack_seq) {
                    connection_stats.acks_received++;
                    ESP_LOGI(TAG, "ACK received for seq %d", expected_ack_seq);
                    return ESP_OK;
                }
            }
        }

        ESP_LOGW(TAG, "No ACK received, attempt %d/%d", attempt + 1, max_retries + 1);
    }

    ESP_LOGE(TAG, "Failed to get ACK after %d attempts", max_retries + 1);
    connection_stats.failed_transmissions++;
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

    lora_packet_t *packet = (lora_packet_t *)rx_buffer;

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
    ret = calculate_mac(mac_data, 18, sender_device.aes_key, calculated_mac);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MAC calculation failed");
        return ESP_FAIL;
    }

    if (memcmp(packet->mac, calculated_mac, LORA_MAC_SIZE) != 0) {
        ESP_LOGW(TAG, "MAC verification failed for device 0x%04X", packet->device_id);
        return ESP_ERR_INVALID_CRC;
    }

    // Decrypt data using sender's AES-256 key
    uint8_t plaintext[16];
    mbedtls_aes_context sender_aes_ctx;
    mbedtls_aes_init(&sender_aes_ctx);
    mbedtls_aes_setkey_dec(&sender_aes_ctx, sender_device.aes_key, 256);
    mbedtls_aes_crypt_ecb(&sender_aes_ctx, MBEDTLS_AES_DECRYPT, packet->encrypted_data, plaintext);
    mbedtls_aes_free(&sender_aes_ctx);

    // Parse decrypted data
    packet_data->device_id      = packet->device_id;
    packet_data->sequence_num   = (plaintext[0] << 8) | plaintext[1];
    packet_data->command        = plaintext[2];
    packet_data->payload_length = plaintext[3];

    if (packet_data->payload_length > LORA_PAYLOAD_MAX_SIZE) {
        ESP_LOGW(TAG, "Invalid payload length: %d", packet_data->payload_length);
        return ESP_ERR_INVALID_SIZE;
    }

    if (packet_data->payload_length > 0) {
        memcpy(packet_data->payload, &plaintext[4], packet_data->payload_length);
    }

    // Sliding window deduplication with bitmap (enterprise-grade)
    int32_t seq_diff = (int32_t)packet_data->sequence_num - (int32_t)sender_device.highest_sequence;

    if (seq_diff > 0) {
        // New packet, higher than anything seen
        if (seq_diff < 64) {
            // Shift bitmap and mark this packet
            sender_device.recent_bitmap <<= seq_diff;
            sender_device.recent_bitmap |= 1;
        } else {
            // Large gap (reboot or wrap-around), reset bitmap
            ESP_LOGI(TAG, "Large sequence gap detected for 0x%04X, resetting window", packet_data->device_id);
            sender_device.recent_bitmap = 1;
        }
        sender_device.highest_sequence = packet_data->sequence_num;

    } else if (seq_diff == 0) {
        // Exact duplicate of highest sequence
        ESP_LOGW(TAG, "Duplicate packet from 0x%04X: seq %d", packet_data->device_id, packet_data->sequence_num);
        return ESP_ERR_INVALID_STATE;

    } else if (seq_diff > -64) {
        // Within recent window (out-of-order packet)
        uint8_t bit_pos = -seq_diff;
        if (sender_device.recent_bitmap & (1ULL << bit_pos)) {
            ESP_LOGW(TAG, "Duplicate packet from 0x%04X: seq %d (already seen)", packet_data->device_id,
                     packet_data->sequence_num);
            return ESP_ERR_INVALID_STATE;
        }
        // Mark as seen
        sender_device.recent_bitmap |= (1ULL << bit_pos);
        ESP_LOGD(TAG, "Out-of-order packet accepted from 0x%04X: seq %d", packet_data->device_id,
                 packet_data->sequence_num);

    } else {
        // Too old (>64 packets behind), likely reboot or major packet loss
        ESP_LOGI(TAG, "Very old packet from 0x%04X (seq %d vs %d), accepting as reboot", packet_data->device_id,
                 packet_data->sequence_num, sender_device.highest_sequence);
        sender_device.highest_sequence = packet_data->sequence_num;
        sender_device.recent_bitmap    = 1;
    }

    // Update device registry with new sequence tracking (RAM-only, not persisted)
    device_registry_update_sequence(packet_data->device_id, sender_device.highest_sequence,
                                    sender_device.recent_bitmap);

    // Track received packets
    connection_stats.packets_received++;

    // Send ACK for non-ACK packets
    if (packet_data->command != CMD_ACK) {
        esp_err_t ack_ret = lora_protocol_send_ack(packet_data->device_id, packet_data->sequence_num);
        if (ack_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send ACK: %s", esp_err_to_name(ack_ret));
        }
    }

    // Update RSSI and timestamp for connection monitoring
    last_rssi        = lora_get_rssi();
    last_packet_time = esp_timer_get_time();

    ESP_LOGI(TAG, "Valid packet from %s (0x%04X): cmd=0x%02X, seq=%d, RSSI=%d dBm", sender_device.device_name,
             packet_data->device_id, packet_data->command, packet_data->sequence_num, last_rssi);

    return ESP_OK;
}

esp_err_t lora_protocol_send_ack(uint16_t to_device_id, uint16_t ack_sequence_num)
{
    const uint8_t ack_payload[2] = {(ack_sequence_num >> 8) & 0xFF, ack_sequence_num & 0xFF};

    return lora_protocol_send_command(CMD_ACK, ack_payload, 2);
}

uint16_t lora_protocol_get_next_sequence(void)
{
    return sequence_counter + 1;
}

lora_connection_state_t lora_protocol_get_connection_state(void)
{
    uint64_t now                    = esp_timer_get_time();
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
            const char *state_names[] = {"EXCELLENT", "GOOD", "WEAK", "POOR", "LOST"};
            ESP_LOGI(TAG, "Connection state: %s (RSSI: %d dBm)", state_names[state], last_rssi);
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
    BaseType_t ret       = xTaskCreate(rssi_monitor_task, "lora_rssi", 3072, NULL, 3, &rssi_monitor_task_handle);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RSSI monitor task");
        rssi_monitor_running = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "RSSI monitor started");
    return ESP_OK;
}

esp_err_t lora_protocol_get_stats(lora_connection_stats_t *stats)
{
    if (!stats) {
        return ESP_ERR_INVALID_ARG;
    }

    *stats = connection_stats;

    // Calculate packet loss rate
    if (connection_stats.packets_sent > 0) {
        stats->packet_loss_rate = (float)connection_stats.failed_transmissions / connection_stats.packets_sent * 100.0f;
    } else {
        stats->packet_loss_rate = 0.0f;
    }

    return ESP_OK;
}

void lora_protocol_reset_stats(void)
{
    memset(&connection_stats, 0, sizeof(connection_stats));
    ESP_LOGI(TAG, "Connection statistics reset");
}
