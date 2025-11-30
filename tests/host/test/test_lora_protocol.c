/**
 * @file test_lora_protocol.c
 * @brief Unit tests for lora_protocol using Unity/CMock
 */

#include "unity.h"
#include "lora_protocol.h"
#include "mock_lora_driver.h"
#include "mock_device_registry.h"
#include "mock_general_config.h"
#include "mock_power_mgmt.h"
#include "mock_esp_system.h"

// Test fixtures
static uint8_t test_aes_key[32];
static uint16_t test_device_id;
static uint8_t captured_packet[256];
static size_t captured_packet_len;

void setUp(void)
{
    // Initialize test data
    test_device_id = 0x1234;
    for (int i = 0; i < 32; i++) {
        test_aes_key[i] = i;
    }
    captured_packet_len = 0;
}

void tearDown(void)
{
    // Cleanup after each test
}

// Helper to capture sent packets
static esp_err_t capture_packet(const uint8_t *data, size_t length, int num_calls)
{
    (void)num_calls;
    if (length <= sizeof(captured_packet)) {
        memcpy(captured_packet, data, length);
        captured_packet_len = length;
    }
    return ESP_OK;
}

void test_lora_protocol_init_success(void)
{
    lora_driver_init_ExpectAndReturn(ESP_OK);
    
    esp_err_t ret = lora_protocol_init(test_device_id, test_aes_key);
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void test_lora_protocol_init_fails_with_driver_error(void)
{
    lora_driver_init_ExpectAndReturn(ESP_FAIL);
    
    esp_err_t ret = lora_protocol_init(test_device_id, test_aes_key);
    
    TEST_ASSERT_EQUAL(ESP_FAIL, ret);
}

void test_lora_protocol_send_keyboard_creates_valid_packet(void)
{
    // Arrange
    lora_driver_init_ExpectAndReturn(ESP_OK);
    lora_protocol_init(test_device_id, test_aes_key);
    
    lora_send_packet_Stub(capture_packet);
    
    // Act
    esp_err_t ret = lora_protocol_send_keyboard(1, 0x00, 0x4F);
    
    // Assert
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(22, captured_packet_len); // Header(2) + IV(16) + Encrypted(4)
}

void test_lora_protocol_send_keyboard_multiple_keys(void)
{
    // Arrange
    lora_driver_init_ExpectAndReturn(ESP_OK);
    lora_protocol_init(test_device_id, test_aes_key);
    
    lora_send_packet_Stub(capture_packet);
    
    // Act - Send multiple different keys
    uint8_t keys[] = {0x4F, 0x50, 0x2C, 0x29, 0x3E};
    for (int i = 0; i < 5; i++) {
        esp_err_t ret = lora_protocol_send_keyboard(1, 0x00, keys[i]);
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        TEST_ASSERT_EQUAL(22, captured_packet_len);
    }
}

void test_lora_protocol_receive_packet_success(void)
{
    // Arrange - Create a valid encrypted packet
    lora_driver_init_ExpectAndReturn(ESP_OK);
    lora_protocol_init(test_device_id, test_aes_key);
    
    lora_send_packet_Stub(capture_packet);
    lora_protocol_send_keyboard(1, 0x00, 0x4F);
    
    // Setup receive mock
    uint8_t rx_buffer[256];
    size_t rx_len = captured_packet_len;
    memcpy(rx_buffer, captured_packet, rx_len);
    
    lora_receive_packet_ExpectAnyArgsAndReturn(ESP_OK);
    lora_receive_packet_ReturnArrayThruPtr_data(rx_buffer, rx_len);
    lora_receive_packet_ReturnThruPtr_length(&rx_len);
    lora_get_last_rssi_ExpectAndReturn(-50);
    power_mgmt_update_activity_ExpectAndReturn(ESP_OK);
    
    // Act
    lora_packet_data_t packet_data;
    esp_err_t ret = lora_protocol_receive_packet(&packet_data, 100);
    
    // Assert
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(test_device_id, packet_data.device_id);
    TEST_ASSERT_EQUAL(CMD_HID_REPORT, packet_data.command);
    TEST_ASSERT_EQUAL(-50, packet_data.rssi);
}

void test_lora_protocol_receive_timeout(void)
{
    // Arrange
    lora_driver_init_ExpectAndReturn(ESP_OK);
    lora_protocol_init(test_device_id, test_aes_key);
    
    lora_receive_packet_ExpectAnyArgsAndReturn(ESP_ERR_TIMEOUT);
    
    // Act
    lora_packet_data_t packet_data;
    esp_err_t ret = lora_protocol_receive_packet(&packet_data, 100);
    
    // Assert
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, ret);
}

void test_lora_protocol_receive_invalid_packet_length(void)
{
    // Arrange
    lora_driver_init_ExpectAndReturn(ESP_OK);
    lora_protocol_init(test_device_id, test_aes_key);
    
    uint8_t short_packet[10] = {0};
    size_t short_len = 10;
    
    lora_receive_packet_ExpectAnyArgsAndReturn(ESP_OK);
    lora_receive_packet_ReturnArrayThruPtr_data(short_packet, short_len);
    lora_receive_packet_ReturnThruPtr_length(&short_len);
    
    // Act
    lora_packet_data_t packet_data;
    esp_err_t ret = lora_protocol_receive_packet(&packet_data, 100);
    
    // Assert
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

void test_lora_protocol_send_without_init_fails(void)
{
    // Act - Try to send without initialization
    esp_err_t ret = lora_protocol_send_keyboard(1, 0x00, 0x4F);
    
    // Assert
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}
