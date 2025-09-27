/**
 * @file web_config.h
 * @brief Web configuration interface with Wi-Fi AP and HTTP server
 * 
 * CONTEXT: Ticket 6.1 - Web Configuration Interface
 * PURPOSE: Wi-Fi AP mode with HTTP server for device configuration
 * FEATURES: Settings management, OTA updates, device status
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WEB_CONFIG_SSID_MAX_LEN     32
#define WEB_CONFIG_PASSWORD_MAX_LEN 64
#define WEB_CONFIG_DEFAULT_SSID     "LoRaCue-Config"
#define WEB_CONFIG_DEFAULT_PASSWORD "loracue123"
#define WEB_CONFIG_SERVER_PORT      80

/**
 * @brief Web configuration state
 */
typedef enum {
    WEB_CONFIG_STOPPED,     ///< Web config not running
    WEB_CONFIG_STARTING,    ///< Starting Wi-Fi AP and HTTP server
    WEB_CONFIG_RUNNING,     ///< Web config active and accessible
    WEB_CONFIG_ERROR,       ///< Error state
} web_config_state_t;

/**
 * @brief Web configuration settings
 */
typedef struct {
    char ap_ssid[WEB_CONFIG_SSID_MAX_LEN];         ///< AP SSID
    char ap_password[WEB_CONFIG_PASSWORD_MAX_LEN]; ///< AP password
    uint8_t ap_channel;                            ///< AP channel (1-13)
    uint8_t max_connections;                       ///< Max concurrent connections
    uint16_t server_port;                          ///< HTTP server port
    bool enable_ota;                               ///< Enable OTA updates
} web_config_settings_t;

/**
 * @brief Device configuration structure
 */
typedef struct {
    char device_name[32];           ///< Device name
    uint8_t lora_power;             ///< LoRa TX power (0-22 dBm)
    uint32_t lora_frequency;        ///< LoRa frequency in Hz
    uint8_t lora_spreading_factor;  ///< LoRa spreading factor (7-12)
    uint32_t sleep_timeout_ms;      ///< Sleep timeout in milliseconds
    bool auto_sleep_enabled;        ///< Auto sleep enabled
    uint8_t display_brightness;     ///< Display brightness (0-255)
} device_config_t;

/**
 * @brief Initialize web configuration system
 * 
 * @param settings Web config settings (NULL for defaults)
 * @return ESP_OK on success
 */
esp_err_t web_config_init(const web_config_settings_t *settings);

/**
 * @brief Start web configuration mode
 * 
 * @return ESP_OK on success
 */
esp_err_t web_config_start(void);

/**
 * @brief Stop web configuration mode
 * 
 * @return ESP_OK on success
 */
esp_err_t web_config_stop(void);

/**
 * @brief Get current web config state
 * 
 * @return Current state
 */
web_config_state_t web_config_get_state(void);

/**
 * @brief Get current device configuration
 * 
 * @param config Output device configuration
 * @return ESP_OK on success
 */
esp_err_t web_config_get_device_config(device_config_t *config);

/**
 * @brief Set device configuration
 * 
 * @param config New device configuration
 * @return ESP_OK on success
 */
esp_err_t web_config_set_device_config(const device_config_t *config);

/**
 * @brief Get AP connection info
 * 
 * @param ssid Output SSID buffer (min 33 bytes)
 * @param password Output password buffer (min 65 bytes)
 * @param ip_address Output IP address string (min 16 bytes)
 * @return ESP_OK on success
 */
esp_err_t web_config_get_ap_info(char *ssid, char *password, char *ip_address);

/**
 * @brief Check if clients are connected
 * 
 * @return Number of connected clients
 */
uint8_t web_config_get_client_count(void);

#ifdef __cplusplus
}
#endif
