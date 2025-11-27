/**
 * @file test_lora_integration.c
 * @brief Real integration test using actual lora_protocol.c code
 * 
 * This test uses the REAL codebase functions, not duplicated logic.
 */

#include "lora_protocol.h"
#include "lora_driver.h"
#include "unity.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>

static const char *TAG = "TEST_INTEGRATION";

// Mock packet buffer for testing
static uint8_t mock_tx_buffer[256];
static size_t mock_tx_length = 0;
static uint8_t mock_rx_buffer[256];
static size_t mock_rx_length = 0;

// Mock lora_driver functions
esp_err_t lora_driver_init(void) {
    return ESP_OK;
}

esp_err_t lora_send_packet(const uint8_t *data, size_t length) {
    if (length > sizeof(mock_tx_buffer)) return ESP_ERR_INVALID_SIZE;
    memcpy(mock_tx_buffer, data, length);
    mock_tx_length = length;
    ESP_LOGI(TAG, "Mock TX: %d bytes", length);
    return ESP_OK;
}

esp_err_t lora_receive_packet(uint8_t *data, size_t *length, uint32_t timeout_ms) {
    if (mock_rx_length == 0) {
        return ESP_ERR_NOT_FOUND;
    }
    memcpy(data, mock_rx_buffer, mock_rx_length);
    *length = mock_rx_length;
    ESP_LOGI(TAG, "Mock RX: %d bytes", mock_rx_length);
    mock_rx_length = 0; // Consume packet
    return ESP_OK;
}

int16_t lora_get_last_rssi(void) {
    return -50; // Mock RSSI
}

esp_err_t lora_get_config(lora_config_t *config) {
    return ESP_OK;
}

// Test: Real packet creation and encryption
TEST_CASE("Real packet encryption using lora_protocol_send", "[lora_integration]")
{
    uint8_t aes_key[32];
    esp_fill_random(aes_key, 32);
    uint16_t device_id = 0x1234;

    // Initialize protocol with real function
    esp_err_t ret = lora_protocol_init(device_id, aes_key);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Send keyboard packet using real function
    ret = lora_protocol_send_keyboard(1, 0x00, 0x4F); // Right Arrow
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify packet was created
    TEST_ASSERT_GREATER_THAN(0, mock_tx_length);
    TEST_ASSERT_EQUAL(22, mock_tx_length); // Expected packet size

    ESP_LOGI(TAG, "✓ Real packet created: %d bytes", mock_tx_length);
    ESP_LOG_BUFFER_HEX(TAG, mock_tx_buffer, mock_tx_length);
}

// Test: Full round-trip with real code
TEST_CASE("Full encrypt/decrypt round-trip with real code", "[lora_integration]")
{
    uint8_t aes_key[32];
    esp_fill_random(aes_key, 32);
    uint16_t sender_id = 0xABCD;
    uint16_t receiver_id = 0x5678;

    // Sender: Initialize and send
    esp_err_t ret = lora_protocol_init(sender_id, aes_key);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    ret = lora_protocol_send_keyboard(1, 0x00, 0x4F);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Capture sent packet
    uint8_t sent_packet[22];
    memcpy(sent_packet, mock_tx_buffer, mock_tx_length);
    size_t sent_length = mock_tx_length;

    ESP_LOGI(TAG, "Sender (0x%04X) sent %d bytes", sender_id, sent_length);

    // Receiver: Setup to receive
    memcpy(mock_rx_buffer, sent_packet, sent_length);
    mock_rx_length = sent_length;

    // Receiver needs same key to decrypt
    ret = lora_protocol_init(receiver_id, aes_key);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Receive and decrypt
    lora_packet_data_t packet_data;
    ret = lora_protocol_receive_packet(&packet_data, 100);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify decrypted data
    TEST_ASSERT_EQUAL(sender_id, packet_data.device_id);
    TEST_ASSERT_EQUAL(CMD_HID_REPORT, packet_data.command);

    ESP_LOGI(TAG, "✓ Receiver (0x%04X) decrypted from device 0x%04X", 
             receiver_id, packet_data.device_id);
    ESP_LOGI(TAG, "✓ Command: 0x%02X, Payload: %d bytes", 
             packet_data.command, packet_data.payload_length);
}

// Test: MAC verification fails with wrong key
TEST_CASE("MAC verification fails with wrong key (security)", "[lora_integration]")
{
    uint8_t key1[32], key2[32];
    esp_fill_random(key1, 32);
    esp_fill_random(key2, 32);
    uint16_t device_id = 0x1111;

    // Sender with key1
    esp_err_t ret = lora_protocol_init(device_id, key1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    ret = lora_protocol_send_keyboard(1, 0x00, 0x4F);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Capture packet
    memcpy(mock_rx_buffer, mock_tx_buffer, mock_tx_length);
    mock_rx_length = mock_tx_length;

    // Receiver with key2 (wrong key)
    ret = lora_protocol_init(device_id, key2);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Try to receive - should fail MAC verification
    lora_packet_data_t packet_data;
    ret = lora_protocol_receive_packet(&packet_data, 100);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);

    ESP_LOGI(TAG, "✓ MAC verification correctly rejected wrong key");
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "LoRa Protocol Integration Tests");
    ESP_LOGI(TAG, "Using REAL codebase functions");
    ESP_LOGI(TAG, "========================================");

    UNITY_BEGIN();
    
    RUN_TEST(test_real_packet_encryption_using_lora_protocol_send);
    RUN_TEST(test_full_encrypt_decrypt_round_trip_with_real_code);
    RUN_TEST(test_mac_verification_fails_with_wrong_key_security);
    
    UNITY_END();
}
