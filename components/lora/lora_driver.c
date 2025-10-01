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
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#ifdef SIMULATOR_BUILD
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#endif

static const char *TAG = "LORA_DRIVER";

// LoRa configuration
static lora_config_t current_config = {
    .frequency = 915000000,     // 915MHz
    .spreading_factor = 7,      // SF7 for low latency
    .bandwidth = 500,           // 500kHz for high throughput
    .coding_rate = 5,           // 4/5 coding rate
    .tx_power = 14              // 14dBm
};

#ifdef SIMULATOR_BUILD
// WiFi UDP simulation state
static int udp_socket = -1;
static struct sockaddr_in broadcast_addr;
static bool lora_sim_wifi_enabled = false;
static esp_netif_t* ap_netif = NULL;

#define WIFI_SSID "LoRaCue-Sim"
#define WIFI_PASS "simulator"
#define UDP_PORT 8080
#define BROADCAST_IP "192.168.4.255"

// Forward declarations
static esp_err_t lora_sim_wifi_deinit(void);

// LoRa WiFi simulation methods
static esp_err_t lora_sim_wifi_init(void)
{
    if (lora_sim_wifi_enabled) {
        ESP_LOGW(TAG, "LoRa WiFi simulation already enabled");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "🌐 Starting LoRa WiFi simulation");
    
    // Initialize netif if not already done
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Check if WiFi is already initialized
    wifi_mode_t current_mode;
    esp_err_t wifi_status = esp_wifi_get_mode(&current_mode);
    
    if (wifi_status == ESP_OK) {
        ESP_LOGI(TAG, "WiFi already initialized, reusing existing infrastructure");
        // WiFi is already running, just create UDP socket
    } else {
        ESP_LOGI(TAG, "Initializing new WiFi AP for simulation");
        
        // Initialize WiFi
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
            return ret;
        }
        
        // Check if AP netif already exists
        ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (!ap_netif) {
            ESP_LOGI(TAG, "Creating custom WiFi AP netif");
            // Create custom netif instead of default to avoid conflicts
            esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_AP();
            ap_netif = esp_netif_new(&netif_config);
            if (!ap_netif) {
                ESP_LOGE(TAG, "Failed to create custom AP netif");
                return ESP_FAIL;
            }
            
            // Attach to WiFi
            esp_err_t attach_ret = esp_netif_attach_wifi_ap(ap_netif);
            if (attach_ret != ESP_OK && attach_ret != ESP_ERR_INVALID_STATE) {
                ESP_LOGE(TAG, "Failed to attach netif to WiFi: %s", esp_err_to_name(attach_ret));
                esp_netif_destroy(ap_netif);
                ap_netif = NULL;
                return ESP_FAIL;
            }
        } else {
            ESP_LOGI(TAG, "Reusing existing WiFi AP netif");
        }
        
        // Configure and start AP only if not already running
        if (current_mode != WIFI_MODE_AP && current_mode != WIFI_MODE_APSTA) {
            wifi_config_t wifi_config = {
                .ap = {
                    .ssid = WIFI_SSID,
                    .password = WIFI_PASS,
                    .ssid_len = strlen(WIFI_SSID),
                    .channel = 6,
                    .max_connection = 10,
                    .authmode = WIFI_AUTH_WPA2_PSK,
                },
            };
            
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_start());
            
            // Wait for AP to be ready
            vTaskDelay(pdMS_TO_TICKS(2000));
        } else {
            ESP_LOGI(TAG, "WiFi AP already running");
        }
    }
    
    // Create UDP socket (always needed)
    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket < 0) {
        ESP_LOGE(TAG, "Failed to create UDP socket");
        return ESP_FAIL;
    }
    
    // Enable broadcast
    int broadcast = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        ESP_LOGE(TAG, "Failed to enable broadcast");
        close(udp_socket);
        udp_socket = -1;
        return ESP_FAIL;
    }
    
    // Setup broadcast address
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, BROADCAST_IP, &broadcast_addr.sin_addr);
    
    // Bind socket for receiving
    struct sockaddr_in bind_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(UDP_PORT),
    };
    
    if (bind(udp_socket, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind UDP socket");
        close(udp_socket);
        udp_socket = -1;
        return ESP_FAIL;
    }
    
    lora_sim_wifi_enabled = true;
    ESP_LOGI(TAG, "✅ LoRa WiFi simulation ready - Broadcasting on %s:%d", BROADCAST_IP, UDP_PORT);
    
    return ESP_OK;
}

static esp_err_t lora_sim_wifi_deinit(void)
{
    if (!lora_sim_wifi_enabled) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "🔌 Stopping LoRa WiFi simulation");
    
    // Close UDP socket
    if (udp_socket >= 0) {
        close(udp_socket);
        udp_socket = -1;
    }
    
    // Stop WiFi
    esp_wifi_stop();
    esp_wifi_deinit();
    
    // Destroy netif
    if (ap_netif) {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }
    
    lora_sim_wifi_enabled = false;
    ESP_LOGI(TAG, "✅ LoRa WiFi simulation stopped");
    
    return ESP_OK;
}

static esp_err_t lora_sim_send_packet(const uint8_t *data, size_t length)
{
    if (!lora_sim_wifi_enabled || udp_socket < 0) {
        ESP_LOGW(TAG, "LoRa WiFi simulation not available (config mode active?)");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Send UDP broadcast packet
    int sent = sendto(udp_socket, data, length, 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
    if (sent < 0) {
        ESP_LOGE(TAG, "UDP broadcast failed");
        return ESP_FAIL;
    }
    
    ESP_LOGD(TAG, "📡 UDP broadcast: %d bytes", sent);
    return ESP_OK;
}

static esp_err_t lora_sim_receive_packet(uint8_t *data, size_t max_length, size_t *received_length, uint32_t timeout_ms)
{
    if (!lora_sim_wifi_enabled || udp_socket < 0) {
        ESP_LOGW(TAG, "LoRa WiFi simulation not available (config mode active?)");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Set socket timeout
    struct timeval timeout = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
    };
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // Receive UDP packet
    int received = recvfrom(udp_socket, data, max_length, 0, NULL, NULL);
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return ESP_ERR_TIMEOUT;
        }
        ESP_LOGE(TAG, "UDP receive failed: %d", errno);
        return ESP_FAIL;
    }
    
    *received_length = received;
    ESP_LOGD(TAG, "📥 UDP received: %d bytes", received);
    return ESP_OK;
}

// Public API for config mode management
esp_err_t lora_driver_disable_sim_wifi(void)
{
    ESP_LOGI(TAG, "Disabling LoRa WiFi simulation for config mode");
    return lora_sim_wifi_deinit();
}

esp_err_t lora_driver_enable_sim_wifi(void)
{
    ESP_LOGI(TAG, "Re-enabling LoRa WiFi simulation after config mode");
    return lora_sim_wifi_init();
}

bool lora_driver_is_sim_wifi_enabled(void)
{
    return lora_sim_wifi_enabled;
}
#endif

esp_err_t lora_driver_init(void)
{
    // Load LoRa config from NVS (or use defaults)
    lora_config_t config;
    lora_get_config(&config);
    current_config = config;
    
    ESP_LOGI(TAG, "LoRa config: %lu Hz, SF%d, %d kHz, %d dBm", 
             config.frequency, config.spreading_factor, config.bandwidth, config.tx_power);
    
#ifdef SIMULATOR_BUILD
    ESP_LOGI(TAG, "🌐 Simulator mode: Initializing WiFi UDP transport");
    ESP_LOGI(TAG, "🌐 Skipping SX1262 hardware initialization (simulated)");
    return lora_sim_wifi_init();
#else
    ESP_LOGI(TAG, "📻 Hardware mode: Initializing SX1262 LoRa");
    
    // Reset SX1262
    esp_err_t ret = bsp_sx1262_reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SX1262 reset failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Read version to verify communication
    uint8_t version = bsp_sx1262_read_register(0x0320);
    if (version != 0x14) {
        ESP_LOGE(TAG, "SX1262 version check failed: 0x%02x (expected 0x14)", version);
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "SX1262 detected successfully (version: 0x%02x)", version);
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
    // Hardware LoRa transmission - placeholder
    ESP_LOGI(TAG, "📻 LoRa TX: %d bytes", length);
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
    // Hardware LoRa reception - placeholder
    *received_length = 0;
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
    
    // Try to load from NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("lora", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        size_t required_size = sizeof(lora_config_t);
        ret = nvs_get_blob(nvs_handle, "config", config, &required_size);
        nvs_close(nvs_handle);
        
        if (ret == ESP_OK) {
            ESP_LOGD(TAG, "LoRa config loaded from NVS");
            return ESP_OK;
        }
    }
    
    // Use current config if NVS read failed
    *config = current_config;
    ESP_LOGD(TAG, "Using current LoRa configuration");
    return ESP_OK;
}

esp_err_t lora_set_config(const lora_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Save to NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("lora", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open LoRa NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_set_blob(nvs_handle, "config", config, sizeof(lora_config_t));
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }
    
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        // Update current config
        current_config = *config;
        ESP_LOGI(TAG, "LoRa configuration saved to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to save LoRa config: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t lora_set_receive_mode(void)
{
#ifdef SIMULATOR_BUILD
    // UDP is always in receive mode
    return ESP_OK;
#else
    // Hardware receive mode - placeholder
    ESP_LOGI(TAG, "📻 LoRa RX mode");
    return ESP_OK;
#endif
}
