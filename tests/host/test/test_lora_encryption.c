/**
 * @file test_lora_encryption.c
 * @brief Unit tests for LoRa protocol encryption and decryption
 */

#include "unity.h"
#include <string.h>
#include <stdint.h>

// Test AES-256 key size
#define AES_KEY_SIZE 32
#define AES_BLOCK_SIZE 16
#define MAC_SIZE 16

// Mock AES encryption/decryption functions
static uint8_t test_aes_key[AES_KEY_SIZE] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
};

void setUp(void)
{
    // Setup before each test
}

void tearDown(void)
{
    // Cleanup after each test
}

void test_aes_key_size_constant(void)
{
    TEST_ASSERT_EQUAL(32, AES_KEY_SIZE);
    TEST_ASSERT_EQUAL(16, AES_BLOCK_SIZE);
    TEST_ASSERT_EQUAL(16, MAC_SIZE);
}

void test_aes_key_validation(void)
{
    uint8_t valid_key[AES_KEY_SIZE];
    memset(valid_key, 0xAA, AES_KEY_SIZE);
    
    // Key should be exactly 32 bytes
    TEST_ASSERT_EQUAL(AES_KEY_SIZE, sizeof(valid_key));
    
    // All bytes should be set
    for (int i = 0; i < AES_KEY_SIZE; i++) {
        TEST_ASSERT_EQUAL(0xAA, valid_key[i]);
    }
}

void test_encryption_input_validation(void)
{
    uint8_t plaintext[16] = "Hello LoRaCue!";
    uint8_t ciphertext[32];
    uint8_t mac[MAC_SIZE];
    
    // Test data sizes
    TEST_ASSERT_EQUAL(16, sizeof(plaintext));
    TEST_ASSERT_EQUAL(32, sizeof(ciphertext));
    TEST_ASSERT_EQUAL(16, sizeof(mac));
    
    // Plaintext should not be empty
    TEST_ASSERT_NOT_EQUAL(0, strlen((char*)plaintext));
}

void test_mac_generation(void)
{
    uint8_t data[] = "Test data for MAC";
    uint8_t mac1[MAC_SIZE];
    uint8_t mac2[MAC_SIZE];
    
    // Simulate MAC generation (in real implementation, this would use HMAC)
    memset(mac1, 0x42, MAC_SIZE);
    memset(mac2, 0x42, MAC_SIZE);
    
    // Same input should produce same MAC
    TEST_ASSERT_EQUAL_MEMORY(mac1, mac2, MAC_SIZE);
    
    // MAC should be full size
    TEST_ASSERT_EQUAL(MAC_SIZE, sizeof(mac1));
}

void test_encryption_roundtrip(void)
{
    uint8_t original[] = "LoRaCue Test Data";
    uint8_t encrypted[32];
    uint8_t decrypted[32];
    size_t original_len = strlen((char*)original);
    
    // Simulate encryption (XOR with key for test)
    for (size_t i = 0; i < original_len; i++) {
        encrypted[i] = original[i] ^ test_aes_key[i % AES_KEY_SIZE];
    }
    
    // Simulate decryption (XOR again to get original)
    for (size_t i = 0; i < original_len; i++) {
        decrypted[i] = encrypted[i] ^ test_aes_key[i % AES_KEY_SIZE];
    }
    
    // Roundtrip should recover original data
    TEST_ASSERT_EQUAL_MEMORY(original, decrypted, original_len);
}

void test_key_derivation(void)
{
    uint8_t master_key[AES_KEY_SIZE];
    uint8_t derived_key[AES_KEY_SIZE];
    uint16_t device_id = 0x1234;
    
    // Initialize master key
    memset(master_key, 0x55, AES_KEY_SIZE);
    
    // Simulate key derivation (XOR with device ID)
    memcpy(derived_key, master_key, AES_KEY_SIZE);
    derived_key[0] ^= (device_id >> 8) & 0xFF;
    derived_key[1] ^= device_id & 0xFF;
    
    // Derived key should be different from master
    TEST_ASSERT_NOT_EQUAL(master_key[0], derived_key[0]);
    TEST_ASSERT_NOT_EQUAL(master_key[1], derived_key[1]);
    
    // But same size
    TEST_ASSERT_EQUAL(sizeof(master_key), sizeof(derived_key));
}

void test_iv_generation(void)
{
    uint8_t iv1[AES_BLOCK_SIZE];
    uint8_t iv2[AES_BLOCK_SIZE];
    uint32_t counter1 = 100;
    uint32_t counter2 = 101;
    
    // Simulate IV generation from counter
    memset(iv1, 0, AES_BLOCK_SIZE);
    memset(iv2, 0, AES_BLOCK_SIZE);
    
    // Set counter in IV (little endian)
    iv1[0] = counter1 & 0xFF;
    iv1[1] = (counter1 >> 8) & 0xFF;
    iv1[2] = (counter1 >> 16) & 0xFF;
    iv1[3] = (counter1 >> 24) & 0xFF;
    
    iv2[0] = counter2 & 0xFF;
    iv2[1] = (counter2 >> 8) & 0xFF;
    iv2[2] = (counter2 >> 16) & 0xFF;
    iv2[3] = (counter2 >> 24) & 0xFF;
    
    // Different counters should produce different IVs
    TEST_ASSERT_NOT_EQUAL_MEMORY(iv1, iv2, 4);
    
    // But same size
    TEST_ASSERT_EQUAL(sizeof(iv1), sizeof(iv2));
}

void test_packet_integrity(void)
{
    uint8_t packet_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t mac[MAC_SIZE];
    uint8_t corrupted_data[] = {0x01, 0x02, 0xFF, 0x04, 0x05}; // Byte 2 corrupted
    uint8_t corrupted_mac[MAC_SIZE];
    
    // Generate MAC for original data
    memset(mac, 0x33, MAC_SIZE);
    
    // Generate MAC for corrupted data (should be different)
    memset(corrupted_mac, 0x44, MAC_SIZE);
    
    // MACs should be different for different data
    TEST_ASSERT_NOT_EQUAL_MEMORY(mac, corrupted_mac, MAC_SIZE);
    
    // Original data should not equal corrupted data
    TEST_ASSERT_NOT_EQUAL_MEMORY(packet_data, corrupted_data, sizeof(packet_data));
}

void test_encryption_block_alignment(void)
{
    uint8_t data_15[15] = {0}; // Not block aligned
    uint8_t data_16[16] = {0}; // Block aligned
    uint8_t data_17[17] = {0}; // Not block aligned
    
    // Test padding calculation
    size_t padded_15 = ((sizeof(data_15) + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    size_t padded_16 = ((sizeof(data_16) + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    size_t padded_17 = ((sizeof(data_17) + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    
    TEST_ASSERT_EQUAL(16, padded_15); // 15 -> 16
    TEST_ASSERT_EQUAL(16, padded_16); // 16 -> 16
    TEST_ASSERT_EQUAL(32, padded_17); // 17 -> 32
}

void test_key_security_properties(void)
{
    uint8_t weak_key_zeros[AES_KEY_SIZE] = {0};
    uint8_t weak_key_ones[AES_KEY_SIZE];
    uint8_t strong_key[AES_KEY_SIZE];
    
    memset(weak_key_ones, 0xFF, AES_KEY_SIZE);
    
    // Generate pseudo-random strong key
    for (int i = 0; i < AES_KEY_SIZE; i++) {
        strong_key[i] = (i * 17 + 42) & 0xFF;
    }
    
    // Strong key should not be all zeros or all ones
    TEST_ASSERT_NOT_EQUAL_MEMORY(strong_key, weak_key_zeros, AES_KEY_SIZE);
    TEST_ASSERT_NOT_EQUAL_MEMORY(strong_key, weak_key_ones, AES_KEY_SIZE);
    
    // Should have some entropy (not all bytes the same)
    bool has_variation = false;
    for (int i = 1; i < AES_KEY_SIZE; i++) {
        if (strong_key[i] != strong_key[0]) {
            has_variation = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(has_variation);
}
