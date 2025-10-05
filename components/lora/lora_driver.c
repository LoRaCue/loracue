/**
 * @file lora_driver.c
 * @brief LoRa driver wrapper for LoRaCue presentation clicker
 *
 * CONTEXT: LoRaCue Phase 2 - Core Communication
 * HARDWARE: SX1262 LoRa transceiver via SPI (Hardware) / WiFi UDP (Simulator)
 * CONFIG: SF7/BW500kHz/CR4:5 for <50ms latency
 * PURPOSE: Point-to-point LoRa communication with WiFi simulation support
 */

#include "lora_driver.h"
#include "bsp.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

#ifndef SIMULATOR_BUILD
#include "ra01s.h" // Include RA01S library for hardware
#endif

static const char *TAG = "LORA_DRIVER";

// LoRa configuration
static lora_config_t current_config = {
    .frequency        = 915000000, // 915MHz
    .spreading_factor = 7,         // SF7 for low latency
    .bandwidth        = 500,       // 500kHz for high throughput
    .coding_rate      = 5,         // 4/5 coding rate
    .tx_power         = 14         // 14dBm
};

#ifdef SIMULATOR_BUILD
// Simple simulation state (no WiFi)
static bool lora_sim_initialized = false;

// Simple simulation methods (no WiFi)
static esp_err_t lora_sim_send_packet(const uint8_t *data, size_t length)
{
    ESP_LOGD(TAG, "📡 Simulated send: %d bytes", length);
    return ESP_OK;
}

static esp_err_t lora_sim_receive_packet(uint8_t *data, size_t max_length, size_t *received_length, uint32_t timeout_ms)
{
    // No receiving in simulation
    return ESP_ERR_TIMEOUT;
}
#endif

esp_err_t lora_driver_init(void)
{
    // Load LoRa config from NVS (or use defaults)
    lora_load_config_from_nvs();

    ESP_LOGI(TAG, "LoRa config: %lu Hz, SF%d, %d kHz, %d dBm", current_config.frequency,
             current_config.spreading_factor, current_config.bandwidth, current_config.tx_power);

#ifdef SIMULATOR_BUILD
    ESP_LOGI(TAG, "🌐 Skipping SX1262 hardware initialization (simulated)");
    lora_sim_initialized = true;
    return ESP_OK;
#else
    ESP_LOGI(TAG, "📻 Hardware mode: Initializing SX1262 LoRa");

    // Initialize RA01S library
    LoRaInit();

    int16_t ret = LoRaBegin(current_config.frequency, // Frequency in Hz
                            current_config.tx_power,  // TX power in dBm
                            3.3,                      // TCXO voltage
                            false                     // Use DC-DC regulator (not LDO)
    );

    if (ret != ERR_NONE) {
        ESP_LOGE(TAG, "LoRa initialization failed: %d", ret);
        return ESP_FAIL;
    }

    // Configure LoRa parameters
    LoRaConfig(current_config.spreading_factor, // Spreading factor
               current_config.bandwidth / 125,  // Bandwidth index (125kHz = 0, 250kHz = 1, 500kHz = 2)
               current_config.coding_rate,      // Coding rate
               8,                               // Preamble length
               0,                               // Variable payload length
               true,                            // CRC enabled
               false                            // Normal IQ
    );

    // Set private network sync word (0x1424)
    SetSyncWord(0x1424);

    ESP_LOGI(TAG, "SX1262 initialized successfully");
    return ESP_OK;
#endif
}

esp_err_t lora_send_packet(const uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

#ifdef SIMULATOR_BUILD
    return lora_sim_send_packet(data, length);
#else
    // Hardware LoRa transmission using RA01S
    ESP_LOGI(TAG, "📻 LoRa TX: %d bytes", length);

    bool ret = LoRaSend((uint8_t *)data, length, SX126x_TXMODE_SYNC);
    if (!ret) {
        ESP_LOGE(TAG, "LoRa transmission failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LoRa packet sent successfully");
    return ESP_OK;
#endif
}

esp_err_t lora_receive_packet(uint8_t *data, size_t max_length, size_t *received_length, uint32_t timeout_ms)
{
    if (!data || !received_length || max_length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

#ifdef SIMULATOR_BUILD
    return lora_sim_receive_packet(data, max_length, received_length, timeout_ms);
#else
    // Hardware LoRa reception using RA01S
    *received_length = 0;

    uint8_t bytes_received = LoRaReceive(data, max_length);
    if (bytes_received > 0) {
        *received_length = bytes_received;
        ESP_LOGI(TAG, "📻 LoRa RX: %d bytes", bytes_received);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
#endif
}

int16_t lora_get_rssi(void)
{
#ifdef SIMULATOR_BUILD
    // Simulate RSSI for WiFi (always good signal in simulator)
    return -50; // dBm
#else
    // Hardware RSSI - placeholder
    return -80; // dBm
#endif
}

uint32_t lora_get_frequency(void)
{
    return current_config.frequency;
}

esp_err_t lora_get_config(lora_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    *config = current_config;
    return ESP_OK;
}

esp_err_t lora_load_config_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("lora_config", NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "No LoRa config in NVS, using defaults");
        return ESP_OK; // Use defaults
    }

    size_t required_size = sizeof(lora_config_t);
    ret                  = nvs_get_blob(nvs_handle, "config", &current_config, &required_size);
    nvs_close(nvs_handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Loaded LoRa config from NVS: %lu Hz, SF%d, %d kHz, %d dBm", current_config.frequency,
                 current_config.spreading_factor, current_config.bandwidth, current_config.tx_power);
    } else {
        ESP_LOGI(TAG, "Failed to load LoRa config from NVS, using defaults");
    }

    return ESP_OK;
}

esp_err_t lora_set_config(const lora_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    current_config = *config;

    // Save to NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("lora_config", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        nvs_set_blob(nvs_handle, "config", config, sizeof(lora_config_t));
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "LoRa config updated: %lu Hz, SF%d, %d kHz, %d dBm", config->frequency, config->spreading_factor,
             config->bandwidth, config->tx_power);

#ifndef SIMULATOR_BUILD
    // Reconfigure hardware with new settings
    ESP_LOGI(TAG, "Reconfiguring LoRa hardware with new settings");

    // Re-initialize with new frequency and power
    int16_t init_ret = LoRaBegin(config->frequency, // Frequency in Hz
                                 config->tx_power,  // TX power in dBm
                                 3.3,               // TCXO voltage
                                 false              // Use DC-DC regulator
    );

    if (init_ret != ERR_NONE) {
        ESP_LOGE(TAG, "LoRa reconfiguration failed: %d", init_ret);
        return ESP_FAIL;
    }

    // Update LoRa parameters
    LoRaConfig(config->spreading_factor, // Spreading factor
               config->bandwidth / 125,  // Bandwidth index (125kHz = 0, 250kHz = 1, 500kHz = 2)
               config->coding_rate,      // Coding rate
               8,                        // Preamble length
               0,                        // Variable payload length
               true,                     // CRC enabled
               false                     // Normal IQ
    );

    // Set private network sync word (0x1424)
    SetSyncWord(0x1424);

    // Return to receive mode
    SetRx(0);

    ESP_LOGI(TAG, "LoRa hardware reconfigured successfully");
#endif

    return ESP_OK;
}

esp_err_t lora_set_receive_mode(void)
{
#ifdef SIMULATOR_BUILD
    // UDP is always in receive mode
    return ESP_OK;
#else
    // Hardware receive mode using RA01S
    ESP_LOGI(TAG, "📻 LoRa RX mode");

    // Set LoRa to continuous receive mode
    SetRx(0); // 0 = continuous receive mode

    return ESP_OK;
#endif
}
