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
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lora_bands.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

#ifndef SIMULATOR_BUILD
#include "sx126x.h" // Include SX126x library for hardware
#endif

static const char *TAG = "LORA_DRIVER";

// TX queue for outgoing packets
#define TX_QUEUE_SIZE 8
#define RX_QUEUE_SIZE 8
#define MAX_PACKET_SIZE 255

// cppcheck-suppress unusedStructMember
typedef struct {
    uint8_t data[MAX_PACKET_SIZE];
    size_t length;
} lora_tx_packet_t;

// cppcheck-suppress unusedStructMember
typedef struct {
    uint8_t data[MAX_PACKET_SIZE];
    size_t length;
} lora_rx_packet_t;

static QueueHandle_t tx_queue      = NULL;
static QueueHandle_t rx_queue      = NULL;
static TaskHandle_t tx_task_handle = NULL;
static TaskHandle_t rx_task_handle = NULL;

// LoRa configuration
static lora_config_t current_config = {
    .frequency        = 868100000, // 868.1 MHz default
    .spreading_factor = 7,         // SF7 for low latency
    .bandwidth        = 500,       // 500kHz for high throughput
    .coding_rate      = 5,         // 4/5 coding rate
    .tx_power         = 14,        // 14dBm
    .band_id          = "HW_868"   // Default to 868 MHz band
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
    (void)data;
    (void)max_length;
    (void)received_length;
    (void)timeout_ms;
    // No receiving in simulation
    return ESP_ERR_TIMEOUT;
}
#endif

#ifndef SIMULATOR_BUILD
// TX task - processes queue and transmits packets
static void lora_tx_task(void *arg)
{
    lora_tx_packet_t packet;

    while (1) {
        // Wait for packet in queue
        if (xQueueReceive(tx_queue, &packet, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "📻 LoRa TX: %d bytes", packet.length);

            esp_err_t ret = sx126x_send(packet.data, packet.length, SX126x_TXMODE_SYNC);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "TX failed: %s", esp_err_to_name(ret));
            } else {
                ESP_LOGI(TAG, "TX done");
            }
        }
    }
}

// RX task - continuously polls for incoming packets and enqueues them
static void lora_rx_task(void *arg)
{
    uint8_t rx_buffer[MAX_PACKET_SIZE];
    uint8_t bytes_received;

    while (1) {
        esp_err_t ret = sx126x_receive(rx_buffer, MAX_PACKET_SIZE, &bytes_received);

        if (ret == ESP_OK && bytes_received > 0) {
            ESP_LOGI(TAG, "📻 LoRa RX: %d bytes", bytes_received);

            // Enqueue received packet
            lora_rx_packet_t rx_packet;
            memcpy(rx_packet.data, rx_buffer, bytes_received);
            rx_packet.length = bytes_received;

            if (xQueueSend(rx_queue, &rx_packet, 0) != pdTRUE) {
                ESP_LOGW(TAG, "RX queue full, dropping packet");
            }
        }

        // Poll every 10ms
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif

esp_err_t lora_driver_init(void)
{
#ifndef SIMULATOR_BUILD
    // Create TX queue
    tx_queue = xQueueCreate(TX_QUEUE_SIZE, sizeof(lora_tx_packet_t));
    if (tx_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create TX queue");
        return ESP_ERR_NO_MEM;
    }

    // Create RX queue
    rx_queue = xQueueCreate(RX_QUEUE_SIZE, sizeof(lora_rx_packet_t));
    if (rx_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create RX queue");
        vQueueDelete(tx_queue);
        return ESP_ERR_NO_MEM;
    }
#endif

    // Initialize band profiles from JSON
    esp_err_t ret = lora_bands_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize band profiles");
        return ret;
    }

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

    // Initialize SX126x library
    ret = sx126x_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sx126x_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = sx126x_begin(current_config.frequency, // Frequency in Hz
                       current_config.tx_power,  // TX power in dBm
                       3.3,                      // TCXO voltage
                       false                     // Use DC-DC regulator (not LDO)
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sx126x_begin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure LoRa parameters
    ret = sx126x_config(current_config.spreading_factor, // Spreading factor
                        current_config.bandwidth / 125,  // Bandwidth index (125kHz = 0, 250kHz = 1, 500kHz = 2)
                        current_config.coding_rate,      // Coding rate
                        8,                               // Preamble length
                        0,                               // Variable payload length
                        true,                            // CRC enabled
                        false                            // Normal IQ
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sx126x_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set private network sync word (0x1424)
    SetSyncWord(0x1424);

    // Create TX task
    BaseType_t task_ret = xTaskCreate(lora_tx_task, "lora_tx", 4096, NULL,
                                      5, // Priority
                                      &tx_task_handle);

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create TX task");
        return ESP_FAIL;
    }

    // Create RX task
    task_ret = xTaskCreate(lora_rx_task, "lora_rx", 4096, NULL,
                           5, // Priority
                           &rx_task_handle);

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RX task");
        vTaskDelete(tx_task_handle);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SX1262 initialized successfully with TX/RX tasks");
    return ESP_OK;
#endif
}

esp_err_t lora_send_packet(const uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (length > MAX_PACKET_SIZE) {
        ESP_LOGE(TAG, "Packet too large: %d > %d", length, MAX_PACKET_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }

#ifdef SIMULATOR_BUILD
    return lora_sim_send_packet(data, length);
#else
    // Enqueue packet for TX task
    lora_tx_packet_t packet;
    memcpy(packet.data, data, length);
    packet.length = length;

    if (xQueueSend(tx_queue, &packet, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "TX queue full");
        return ESP_ERR_TIMEOUT;
    }

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
    // Dequeue packet from RX queue (populated by RX task)
    *received_length = 0;

    lora_rx_packet_t rx_packet;
    if (xQueueReceive(rx_queue, &rx_packet, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        size_t copy_len = (rx_packet.length < max_length) ? rx_packet.length : max_length;
        memcpy(data, rx_packet.data, copy_len);
        *received_length = copy_len;

        if (rx_packet.length > max_length) {
            ESP_LOGW(TAG, "RX packet truncated: %d > %d", rx_packet.length, max_length);
        }

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
        // Generate random AES key on first boot
        esp_fill_random(current_config.aes_key, 32);
        ESP_LOGI(TAG, "Generated random AES-256 key");
        return ESP_OK; // Use defaults
    }

    size_t required_size = sizeof(lora_config_t);
    ret                  = nvs_get_blob(nvs_handle, "config", &current_config, &required_size);
    nvs_close(nvs_handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Loaded LoRa config from NVS: %lu Hz, SF%d, %d kHz, %d dBm", current_config.frequency,
                 current_config.spreading_factor, current_config.bandwidth, current_config.tx_power);
        // Check if AES key is all zeros (old config without key)
        bool key_is_zero = true;
        for (int i = 0; i < 32; i++) {
            if (current_config.aes_key[i] != 0) {
                key_is_zero = false;
                break;
            }
        }
        if (key_is_zero) {
            ESP_LOGI(TAG, "AES key not set, generating random key");
            esp_fill_random(current_config.aes_key, 32);
        }
    } else {
        ESP_LOGI(TAG, "Failed to load LoRa config from NVS, using defaults");
        esp_fill_random(current_config.aes_key, 32);
        ESP_LOGI(TAG, "Generated random AES-256 key");
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
    esp_err_t init_ret = sx126x_begin(config->frequency, // Frequency in Hz
                                      config->tx_power,  // TX power in dBm
                                      3.3,               // TCXO voltage
                                      false              // Use DC-DC regulator
    );

    if (init_ret != ESP_OK) {
        ESP_LOGE(TAG, "sx126x_begin failed: %s", esp_err_to_name(init_ret));
        return init_ret;
    }

    // Update LoRa parameters
    init_ret = sx126x_config(config->spreading_factor, // Spreading factor
                             config->bandwidth / 125,  // Bandwidth index (125kHz = 0, 250kHz = 1, 500kHz = 2)
                             config->coding_rate,      // Coding rate
                             8,                        // Preamble length
                             0,                        // Variable payload length
                             true,                     // CRC enabled
                             false                     // Normal IQ
    );

    if (init_ret != ESP_OK) {
        ESP_LOGE(TAG, "sx126x_config failed: %s", esp_err_to_name(init_ret));
        return init_ret;
    }

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
    // Hardware receive mode using SX126x
    ESP_LOGI(TAG, "📻 LoRa RX mode");

    // Set LoRa to continuous receive mode
    SetRx(0); // 0 = continuous receive mode

    return ESP_OK;
#endif
}
