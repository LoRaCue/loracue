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
#include "sx126x.h"
#include "task_config.h"
#include <string.h>

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

/**
 * @brief Convert bandwidth kHz value to SX1262 register value
 * @param bandwidth Bandwidth in kHz
 * @return SX1262 register value
 */
static uint8_t lora_bandwidth_to_register(uint16_t bandwidth)
{
    static const struct {
        uint16_t bw;
        uint8_t reg;
    } bw_table[] = {
        {7, 0x00},   // 7.8 kHz
        {10, 0x08},  // 10.4 kHz
        {15, 0x01},  // 15.6 kHz
        {20, 0x09},  // 20.8 kHz
        {31, 0x02},  // 31.25 kHz
        {41, 0x0A},  // 41.7 kHz
        {62, 0x03},  // 62.5 kHz
        {125, 0x04}, // 125.0 kHz
        {250, 0x05}, // 250.0 kHz
        {500, 0x06}  // 500.0 kHz
    };

    for (size_t i = 0; i < sizeof(bw_table) / sizeof(bw_table[0]); i++) {
        if (bw_table[i].bw == bandwidth) {
            return bw_table[i].reg;
        }
    }
    return 0x04; // Default to 125 kHz
}

// TX task - processes queue and transmits packets
static void lora_tx_task(void *arg)
{
    lora_tx_packet_t packet;

    while (1) {
        // Wait for packet in queue
        if (xQueueReceive(tx_queue, &packet, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "LoRa TX: %d bytes", packet.length);

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
        // Check for TX completion (for interrupt-driven TX)
        sx126x_check_tx_done();

        esp_err_t ret = sx126x_receive(rx_buffer, MAX_PACKET_SIZE, &bytes_received);

        if (ret == ESP_OK && bytes_received > 0) {
            ESP_LOGI(TAG, "LoRa RX: %d bytes", bytes_received);

            // Enqueue received packet
            lora_rx_packet_t rx_packet;
            memcpy(rx_packet.data, rx_buffer, bytes_received);
            rx_packet.length = bytes_received;

            if (xQueueSend(rx_queue, &rx_packet, 0) != pdTRUE) {
                ESP_LOGW(TAG, "RX queue full, dropping packet");
            }
        }

        // Poll every 5ms for better responsiveness
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

esp_err_t lora_driver_init(void)
{
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

    // Initialize regulatory system from JSON
    esp_err_t ret = lora_regulatory_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize regulatory system");
        return ret;
    }

    // Load LoRa config from NVS (or use defaults)
    lora_load_config_from_nvs();

    ESP_LOGI(TAG, "LoRa config: %lu Hz, SF%d, %d kHz, %d dBm", current_config.frequency,
             current_config.spreading_factor, current_config.bandwidth, current_config.tx_power);

    ESP_LOGI(TAG, "Initializing SX1262 LoRa");

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

    // Convert bandwidth kHz to SX1262 register value
    uint8_t bw_reg = lora_bandwidth_to_register(current_config.bandwidth);

    // Configure LoRa parameters
    ret = sx126x_config(current_config.spreading_factor, // Spreading factor
                        bw_reg,                          // Bandwidth register value
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

    // Set private network sync word
    SetSyncWord(LORA_PRIVATE_SYNC_WORD);

    // Create TX task
    BaseType_t task_ret =
        xTaskCreate(lora_tx_task, "lora_tx", TASK_STACK_SIZE_MEDIUM, NULL, TASK_PRIORITY_NORMAL, &tx_task_handle);

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create TX task");
        return ESP_FAIL;
    }

    // Create RX task
    task_ret =
        xTaskCreate(lora_rx_task, "lora_rx", TASK_STACK_SIZE_MEDIUM, NULL, TASK_PRIORITY_NORMAL, &rx_task_handle);

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RX task");
        vTaskDelete(tx_task_handle);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SX1262 initialized successfully with TX/RX tasks");
    return ESP_OK;
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

    // Enqueue packet for TX task
    lora_tx_packet_t packet;
    memcpy(packet.data, data, length);
    packet.length = length;

    if (xQueueSend(tx_queue, &packet, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "TX queue full");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t lora_receive_packet(uint8_t *data, size_t max_length, size_t *received_length, uint32_t timeout_ms)
{
    if (!data || !received_length || max_length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

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
}

int16_t lora_get_rssi(void)
{
    // Read actual RSSI from SX126x chip
    int8_t rssi_packet = 0;
    int8_t snr_packet  = 0;
    GetPacketStatus(&rssi_packet, &snr_packet);
    return (int16_t)rssi_packet;
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
    esp_err_t ret = config_manager_get_lora(&current_config);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to load LoRa config, using defaults");
        return ret;
    }

    ESP_LOGI(TAG, "Loaded LoRa config: %lu Hz, SF%d, %d kHz, %d dBm", current_config.frequency,
             current_config.spreading_factor, current_config.bandwidth, current_config.tx_power);
    
    return ESP_OK;
}

esp_err_t lora_set_config(const lora_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate regulatory domain compliance
    if (strlen(config->regulatory_domain) > 0) {
        const lora_compliance_t *limits = lora_regulatory_get_limits(config->regulatory_domain, config->band_id);
        if (limits) {
            // Check frequency limits
            if (config->frequency < limits->freq_min_khz * 1000 || config->frequency > limits->freq_max_khz * 1000) {
                ESP_LOGE(TAG, "Frequency %lu Hz violates regulatory limits for %s", config->frequency, config->regulatory_domain);
                return ESP_ERR_INVALID_ARG;
            }
            
            // Check power limits
            if (config->tx_power > limits->max_power_dbm) {
                ESP_LOGE(TAG, "TX power %d dBm exceeds regulatory limit %d dBm for %s", 
                         config->tx_power, limits->max_power_dbm, config->regulatory_domain);
                return ESP_ERR_INVALID_ARG;
            }
        }
    }

    current_config = *config;

    // Save via config_manager
    esp_err_t ret = config_manager_set_lora(config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save LoRa config");
        return ret;
    }

    ESP_LOGI(TAG, "LoRa config updated: %lu Hz, SF%d, %d kHz, %d dBm", config->frequency, config->spreading_factor,
             config->bandwidth, config->tx_power);

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

    // Convert bandwidth kHz to SX1262 register value
    uint8_t bw_reg = lora_bandwidth_to_register(config->bandwidth);

    // Update LoRa parameters
    init_ret = sx126x_config(config->spreading_factor, // Spreading factor
                             bw_reg,                   // Bandwidth register value
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

    // Set private network sync word
    SetSyncWord(LORA_PRIVATE_SYNC_WORD);

    // Return to receive mode
    SetRx(0);

    ESP_LOGI(TAG, "LoRa hardware reconfigured successfully");

    return ESP_OK;
}

esp_err_t lora_set_receive_mode(void)
{
    // Hardware receive mode using SX126x
    ESP_LOGI(TAG, "LoRa RX mode");

    // Set LoRa to continuous receive mode
    SetRx(0); // 0 = continuous receive mode

    return ESP_OK;
}
