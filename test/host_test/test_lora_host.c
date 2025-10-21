/**
 * @file test_lora_host.c
 * @brief Host-based test using REAL lora_protocol.c with mocked hardware
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ESP-IDF Mocks
// ============================================================================

#include "esp_err.h"

#define ESP_LOGI(tag, format, ...) printf("[INFO] " format "\n", ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) printf("[ERROR] " format "\n", ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("[WARN] " format "\n", ##__VA_ARGS__)
#define ESP_LOG_BUFFER_HEX(tag, buf, len) do { \
    printf("[HEX] "); \
    for(int i=0; i<len; i++) printf("%02X ", ((uint8_t*)buf)[i]); \
    printf("\n"); \
} while(0)

void esp_fill_random(void *buf, size_t len) {
    uint8_t *p = (uint8_t *)buf;
    for (size_t i = 0; i < len; i++) p[i] = rand() & 0xFF;
}

uint32_t esp_random(void) { return rand(); }
uint64_t esp_timer_get_time(void) { return 0; }

// FreeRTOS mocks
typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
#define portTICK_PERIOD_MS 1
void vTaskDelay(uint32_t ms) {}
void vTaskDelete(void* task) {}

// ============================================================================
// Hardware Mocks (lora_driver, device_registry, etc.)
// ============================================================================

static uint8_t mock_tx_buffer[256];
static size_t mock_tx_length = 0;
static uint8_t mock_rx_buffer[256];
static size_t mock_rx_length = 0;

// Mock lora_driver
typedef struct { uint32_t frequency; uint8_t spreading_factor; uint16_t bandwidth; int8_t tx_power; } lora_config_t;

esp_err_t lora_driver_init(void) { return ESP_OK; }
esp_err_t lora_send_packet(const uint8_t *data, size_t length) {
    if (length > sizeof(mock_tx_buffer)) return ESP_ERR_INVALID_SIZE;
    memcpy(mock_tx_buffer, data, length);
    mock_tx_length = length;
    return ESP_OK;
}
esp_err_t lora_receive_packet(uint8_t *data, size_t *length, uint32_t timeout_ms) {
    if (mock_rx_length == 0) return ESP_ERR_NOT_FOUND;
    memcpy(data, mock_rx_buffer, mock_rx_length);
    *length = mock_rx_length;
    mock_rx_length = 0;
    return ESP_OK;
}
int16_t lora_get_last_rssi(void) { return -50; }
esp_err_t lora_get_config(lora_config_t *config) { return ESP_OK; }

// Mock device_registry
typedef struct { uint16_t device_id; char name[32]; uint8_t mac[6]; uint8_t aes_key[32]; } paired_device_t;
esp_err_t device_registry_init(void) { return ESP_OK; }
esp_err_t device_registry_get(uint16_t device_id, paired_device_t *device) { return ESP_ERR_NOT_FOUND; }

// Mock general_config
typedef enum { DEVICE_MODE_REMOTE, DEVICE_MODE_PC } device_mode_t;
typedef struct { device_mode_t device_mode; } general_config_t;
esp_err_t general_config_get(general_config_t *config) { 
    config->device_mode = DEVICE_MODE_PC;
    return ESP_OK; 
}

// Mock power_mgmt
esp_err_t power_mgmt_update_activity(void) { return ESP_OK; }

// ============================================================================
// Include REAL lora_protocol.c
// ============================================================================

#include "../../components/lora/lora_protocol.c"

// ============================================================================
// Test Framework
// ============================================================================

static int total = 0, passed = 0, failed = 0;

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        printf("❌ FAIL: %s:%d - %s\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("❌ FAIL: %s:%d - Expected %d, got %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        return 1; \
    } \
} while(0)

#define RUN_TEST(test) do { \
    printf("\n🧪 Running: %s\n", #test); \
    if (test() == 0) { \
        printf("✅ PASS: %s\n", #test); \
        passed++; \
    } else { \
        printf("❌ FAIL: %s\n", #test); \
        failed++; \
    } \
    total++; \
} while(0)

// ============================================================================
// Real Integration Tests
// ============================================================================

int test_real_packet_creation(void)
{
    uint8_t aes_key[32];
    esp_fill_random(aes_key, 32);
    uint16_t device_id = 0x1234;

    esp_err_t ret = lora_protocol_init(device_id, aes_key);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    ret = lora_protocol_send_keyboard(1, 0x00, 0x4F); // Right Arrow
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(22, mock_tx_length);

    printf("  ✓ Created 22-byte encrypted packet\n");
    printf("  Device ID: 0x%04X\n", device_id);
    ESP_LOG_BUFFER_HEX("PACKET", mock_tx_buffer, mock_tx_length);

    return 0;
}

int test_encrypt_decrypt_roundtrip(void)
{
    uint8_t aes_key[32];
    esp_fill_random(aes_key, 32);
    uint16_t sender_id = 0xABCD;

    // Sender encrypts
    esp_err_t ret = lora_protocol_init(sender_id, aes_key);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    ret = lora_protocol_send_keyboard(1, 0x00, 0x4F);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Capture packet
    memcpy(mock_rx_buffer, mock_tx_buffer, mock_tx_length);
    mock_rx_length = mock_tx_length;

    // Receiver decrypts (same key)
    ret = lora_protocol_init(0x5678, aes_key);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    lora_packet_data_t packet_data;
    ret = lora_protocol_receive_packet(&packet_data, 100);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(sender_id, packet_data.device_id);
    TEST_ASSERT_EQUAL(CMD_HID_REPORT, packet_data.command);

    printf("  ✓ Encrypted by 0x%04X, decrypted successfully\n", sender_id);
    printf("  ✓ Command: 0x%02X, Payload: %d bytes\n", 
           packet_data.command, packet_data.payload_length);

    return 0;
}

int test_wrong_key_fails(void)
{
    uint8_t key1[32], key2[32];
    esp_fill_random(key1, 32);
    esp_fill_random(key2, 32);

    // Encrypt with key1
    esp_err_t ret = lora_protocol_init(0x1111, key1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    ret = lora_protocol_send_keyboard(1, 0x00, 0x4F);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Try decrypt with key2
    memcpy(mock_rx_buffer, mock_tx_buffer, mock_tx_length);
    mock_rx_length = mock_tx_length;

    ret = lora_protocol_init(0x2222, key2);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    lora_packet_data_t packet_data;
    ret = lora_protocol_receive_packet(&packet_data, 100);
    TEST_ASSERT(ret != ESP_OK); // Should fail

    printf("  ✓ MAC verification correctly rejected wrong key\n");

    return 0;
}

int test_multiple_packets(void)
{
    uint8_t aes_key[32];
    esp_fill_random(aes_key, 32);

    esp_err_t ret = lora_protocol_init(0xCAFE, aes_key);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Send 5 different keypresses
    uint8_t keys[] = {0x4F, 0x50, 0x2C, 0x29, 0x3E}; // Right, Left, Space, Esc, F5
    for (int i = 0; i < 5; i++) {
        ret = lora_protocol_send_keyboard(1, 0x00, keys[i]);
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        TEST_ASSERT_EQUAL(22, mock_tx_length);
    }

    printf("  ✓ Sent 5 encrypted packets successfully\n");

    return 0;
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    printf("========================================\n");
    printf("LoRa Protocol Host-Based Tests\n");
    printf("Using REAL lora_protocol.c code\n");
    printf("Mocked: Hardware only\n");
    printf("========================================\n");

    srand(42);

    RUN_TEST(test_real_packet_creation);
    RUN_TEST(test_encrypt_decrypt_roundtrip);
    RUN_TEST(test_wrong_key_fails);
    RUN_TEST(test_multiple_packets);

    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", total);
    printf("  Passed: %d ✅\n", passed);
    printf("  Failed: %d ❌\n", failed);
    printf("========================================\n");

    return (failed == 0) ? 0 : 1;
}
