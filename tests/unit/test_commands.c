#include "unity.h"
#include "commands_api.h"
#include "general_config.h"
#include "nvs_flash.h"
#include "esp_log.h"

// Mock or stub needed dependencies if not linked?
// Unit tests in ESP-IDF usually link the real components.
// We assume NVS is initialized by the test runner or we do it here.

void setUp(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    general_config_init();
}

void tearDown(void) {
    // general_config_factory_reset(); // Optional cleanup
}

void test_general_config_get_set(void) {
    general_config_t config;
    
    // Set defaults
    strncpy(config.device_name, "TestDevice", sizeof(config.device_name));
    config.device_mode = DEVICE_MODE_PC;
    config.display_contrast = 128;
    config.bluetooth_enabled = true;
    config.bluetooth_pairing_enabled = false;
    config.slot_id = 5;

    TEST_ASSERT_EQUAL(ESP_OK, cmd_set_general_config(&config));

    general_config_t read_config;
    TEST_ASSERT_EQUAL(ESP_OK, cmd_get_general_config(&read_config));

    TEST_ASSERT_EQUAL_STRING("TestDevice", read_config.device_name);
    TEST_ASSERT_EQUAL(DEVICE_MODE_PC, read_config.device_mode);
    TEST_ASSERT_EQUAL(128, read_config.display_contrast);
    TEST_ASSERT_TRUE(read_config.bluetooth_enabled);
    TEST_ASSERT_FALSE(read_config.bluetooth_pairing_enabled);
    TEST_ASSERT_EQUAL(5, read_config.slot_id);
}

void test_lora_config_validation(void) {
    lora_config_t config;
    // Get current valid config to start with
    cmd_get_lora_config(&config);

    // Test invalid bandwidth
    config.bandwidth = 123; // Invalid
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, cmd_set_lora_config(&config));

    // Test valid bandwidth
    config.bandwidth = 125;
    config.frequency = 915000000; // Valid
    TEST_ASSERT_EQUAL(ESP_OK, cmd_set_lora_config(&config));

    // Test invalid frequency alignment
    config.frequency = 915000001; // Not 100kHz aligned
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, cmd_set_lora_config(&config));
}

void test_pairing(void) {
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t key[32] = {0}; // All zeros for test
    const char *name = "MyClicker";

    TEST_ASSERT_EQUAL(ESP_OK, cmd_pair_device(name, mac, key));

    paired_device_t devices[1];
    size_t count = 0;
    TEST_ASSERT_EQUAL(ESP_OK, cmd_get_paired_devices(devices, 1, &count));
    
    // We might have other devices if registry isn't cleared, but we should find ours.
    bool found = false;
    for(size_t i=0; i<count; i++) {
        if (memcmp(devices[i].mac_address, mac, 6) == 0) {
            found = true;
            TEST_ASSERT_EQUAL_STRING(name, devices[i].device_name);
        }
    }
    TEST_ASSERT_TRUE(found);

    TEST_ASSERT_EQUAL(ESP_OK, cmd_unpair_device(mac));
}
