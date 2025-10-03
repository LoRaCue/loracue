/**
 * @file usb_pairing.c
 * @brief Simple USB OTG Device-to-Device Pairing
 *
 * Reuses existing USB CDC pairing protocol by switching presenter to host mode
 */

#include "usb_pairing.h"
#include "cJSON.h"
#include "device_config.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usb/usb_host.h"
#include "usb_cdc.h"

static const char *TAG                        = "usb_pairing";
static bool host_mode_active                  = false;
static TaskHandle_t host_task_handle          = NULL;
static usb_pairing_callback_t result_callback = NULL;

static void usb_host_task(void *arg)
{
    while (host_mode_active) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
    }
    vTaskDelete(NULL);
}

static esp_err_t switch_to_host_mode(void)
{
    if (host_mode_active) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Switching to USB host mode");

    usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags     = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB host: %s", esp_err_to_name(ret));
        return ret;
    }

    host_mode_active = true;
    xTaskCreate(usb_host_task, "usb_host", 4096, NULL, 2, &host_task_handle);

    return ESP_OK;
}

static esp_err_t switch_to_device_mode(void)
{
    if (!host_mode_active) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Switching to USB device mode");

    host_mode_active = false;

    if (host_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        host_task_handle = NULL;
    }

    esp_err_t ret = usb_host_uninstall();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall USB host: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t usb_pairing_start(usb_pairing_callback_t callback)
{
    result_callback = callback;

    device_config_t config;
    device_config_get(&config);

    if (config.device_mode == DEVICE_MODE_PRESENTER) {
        // Presenter becomes host and sends pairing command
        esp_err_t ret = switch_to_host_mode();
        if (ret != ESP_OK) {
            return ret;
        }

        // Generate device info for pairing
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        uint16_t device_id = (mac[4] << 8) | mac[5];

        // Generate AES-256 key (32 bytes)
        uint8_t aes_key[32];
        esp_fill_random(aes_key, sizeof(aes_key));

        // Create pairing JSON
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "name", config.device_name);

        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4],
                 mac[5]);
        cJSON_AddStringToObject(json, "mac", mac_str);

        char key_str[65]; // 32 bytes = 64 hex chars + null
        for (int i = 0; i < 32; i++) {
            snprintf(key_str + i * 2, 3, "%02x", aes_key[i]);
        }
        cJSON_AddStringToObject(json, "key", key_str);

        char *json_string = cJSON_Print(json);

        // Send pairing command via USB CDC (when host mode CDC is implemented)
        ESP_LOGI(TAG, "Would send: PAIR_DEVICE %s", json_string);

        // TODO: Implement USB CDC host mode communication
        // Wait for pairing with timeout
        uint32_t timeout_ms  = 30000; // 30 second timeout
        uint32_t start_time  = esp_timer_get_time() / 1000;
        bool pairing_success = false;

        while ((esp_timer_get_time() / 1000 - start_time) < timeout_ms) {
            // Check for pairing completion
            // For now, simulate timeout
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        free(json_string);
        cJSON_Delete(json);

        if (!pairing_success) {
            ESP_LOGW(TAG, "USB pairing timeout after %d seconds", timeout_ms / 1000);
            if (result_callback) {
                result_callback(false, 0, "Timeout");
            }
            return ESP_ERR_TIMEOUT;
        }

        if (result_callback) {
            result_callback(true, device_id, config.device_name);
        }

    } else {
        // PC device stays in device mode - pairing handled by existing usb_cdc.c
        ESP_LOGI(TAG, "PC device waiting for pairing command");
        if (result_callback) {
            result_callback(true, 0, "Waiting for presenter");
        }
    }

    return ESP_OK;
}

esp_err_t usb_pairing_stop(void)
{
    device_config_t config;
    device_config_get(&config);

    if (config.device_mode == DEVICE_MODE_PRESENTER) {
        switch_to_device_mode();
    }

    ESP_LOGI(TAG, "Pairing stopped");
    return ESP_OK;
}
