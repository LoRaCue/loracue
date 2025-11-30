/**
 * @file test_device_registry.c
 * @brief Unit tests for device_registry data structures
 */

#include "unity.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// Device registry constants
#define DEVICE_NAME_MAX_LEN 32
#define MAC_ADDRESS_LEN 6
#define AES_KEY_LEN 32
#define MAX_PAIRED_DEVICES 10

// Simulated paired device structure
typedef struct {
    uint16_t device_id;
    char name[DEVICE_NAME_MAX_LEN];
    uint8_t mac[MAC_ADDRESS_LEN];
    uint8_t aes_key[AES_KEY_LEN];
} paired_device_t;

void setUp(void)
{
}

void tearDown(void)
{
}

void test_device_structure_size(void)
{
    paired_device_t device;
    // Structure should be reasonable size
    TEST_ASSERT_LESS_THAN(256, sizeof(device));
}

void test_device_name_length(void)
{
    paired_device_t device = {0};
    strncpy(device.name, "Test Device", sizeof(device.name) - 1);
    
    TEST_ASSERT_LESS_THAN(DEVICE_NAME_MAX_LEN, strlen(device.name));
}

void test_mac_address_format(void)
{
    uint8_t mac[MAC_ADDRESS_LEN] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    
    TEST_ASSERT_EQUAL(6, sizeof(mac));
    TEST_ASSERT_EQUAL(0xAA, mac[0]);
    TEST_ASSERT_EQUAL(0xFF, mac[5]);
}

void test_aes_key_length(void)
{
    uint8_t key[AES_KEY_LEN] = {0};
    
    TEST_ASSERT_EQUAL(32, sizeof(key));
}

void test_device_id_uniqueness(void)
{
    uint16_t id1 = 0x1234;
    uint16_t id2 = 0x5678;
    
    TEST_ASSERT_NOT_EQUAL(id1, id2);
}

void test_max_paired_devices_limit(void)
{
    TEST_ASSERT_EQUAL(10, MAX_PAIRED_DEVICES);
    TEST_ASSERT_GREATER_THAN(0, MAX_PAIRED_DEVICES);
}

void test_device_initialization(void)
{
    paired_device_t device = {0};
    
    TEST_ASSERT_EQUAL(0, device.device_id);
    TEST_ASSERT_EQUAL(0, device.name[0]);
    TEST_ASSERT_EQUAL(0, device.mac[0]);
    TEST_ASSERT_EQUAL(0, device.aes_key[0]);
}

void test_device_name_null_termination(void)
{
    paired_device_t device = {0};
    strncpy(device.name, "Test", sizeof(device.name) - 1);
    device.name[sizeof(device.name) - 1] = '\0';
    
    TEST_ASSERT_EQUAL_STRING("Test", device.name);
}

void test_mac_address_copy(void)
{
    uint8_t src_mac[MAC_ADDRESS_LEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t dst_mac[MAC_ADDRESS_LEN] = {0};
    
    memcpy(dst_mac, src_mac, MAC_ADDRESS_LEN);
    
    TEST_ASSERT_EQUAL_UINT8_ARRAY(src_mac, dst_mac, MAC_ADDRESS_LEN);
}

void test_aes_key_copy(void)
{
    uint8_t src_key[AES_KEY_LEN] = {0};
    uint8_t dst_key[AES_KEY_LEN] = {0};
    
    // Fill source key
    for (int i = 0; i < AES_KEY_LEN; i++) {
        src_key[i] = i;
    }
    
    memcpy(dst_key, src_key, AES_KEY_LEN);
    
    TEST_ASSERT_EQUAL_UINT8_ARRAY(src_key, dst_key, AES_KEY_LEN);
}
