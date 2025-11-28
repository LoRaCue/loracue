/**
 * @file usb_pairing.c
 * @brief USB-based device pairing with mode switching
 */

#include "usb_pairing.h"
#include "cJSON.h"
#include "device_registry.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "general_config.h"
#include "lora_driver.h"
#include "tinyusb.h"
#include "usb/cdc_acm_host.h"
#include "usb/usb_host.h"
#include <string.h>

static const char *TAG = "usb_pairing";

static usb_pairing_callback_t result_callback = NULL;
static bool pairing_active                    = false;
static bool pairing_success                   = false;
static uint16_t paired_device_id              = 0;
static char paired_device_name[32]            = {0};
static cdc_acm_dev_hdl_t cdc_device           = NULL;
static usb_host_client_handle_t client_handle = NULL;
static TaskHandle_t usb_host_task_handle      = NULL;
static bool host_mode_active                  = false;

#define PAIRING_TIMEOUT_MS 30000
#define ESP32_VID 0x303A
#define ESP32_PID 0x4002

static void usb_host_lib_task(void *arg)
{
    while (host_mode_active) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
    }
    vTaskDelete(NULL);
}

static void cdc_rx_callback(const uint8_t *data, size_t data_len, void *user_arg)
{
    if (!pairing_active)
        return;

    char response[256];
    size_t copy_len = (data_len < sizeof(response) - 1) ? data_len : sizeof(response) - 1;
    memcpy(response, data, copy_len);
    response[copy_len] = '\0';

    ESP_LOGI(TAG, "Received: %s", response);

    if (strncmp(response, "OK ", 3) == 0) {
        pairing_success  = true;
        char *name_start = strstr(response, "with ");
        if (name_start) {
            name_start += 5;
            const char *id_start = strstr(name_start, " (ID: ");
            if (id_start) {
                size_t name_len = id_start - name_start;
                if (name_len < sizeof(paired_device_name)) {
                    memcpy(paired_device_name, name_start, name_len);
                    paired_device_name[name_len] = '\0';
                }
                sscanf(id_start + 6, "%hx", &paired_device_id);
            }
        }
    }
}

static void new_dev_callback(usb_device_handle_t usb_dev)
{
    ESP_LOGI(TAG, "USB device detected");

    const cdc_acm_host_device_config_t dev_config = {
        .connection_timeout_ms = 5000,
        .out_buffer_size       = 512,
        .event_cb              = NULL,
        .data_cb               = (cdc_acm_data_callback_t)cdc_rx_callback,
        .user_arg              = NULL,
    };

    if (cdc_acm_host_open(ESP32_VID, ESP32_PID, 0, &dev_config, &cdc_device) == ESP_OK) {
        ESP_LOGI(TAG, "CDC-ACM opened");
    }
}

static esp_err_t switch_to_host_mode(void)
{
    ESP_LOGI(TAG, "Switching to USB host mode");

    // Stop USB device mode (CDC) first
    ESP_LOGW(TAG, "=== Uninstalling TinyUSB device mode ===");
    esp_err_t deinit_ret = tinyusb_driver_uninstall();
    ESP_LOGW(TAG, "TinyUSB uninstall result: %s (0x%x)", esp_err_to_name(deinit_ret), deinit_ret);
    
    // Wait longer for USB PHY to be fully released
    ESP_LOGW(TAG, "=== Waiting 500ms for USB PHY release ===");
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGW(TAG, "=== Installing USB host mode ===");
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,  // Let USB host reconfigure PHY for host mode
        .intr_flags     = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t ret = usb_host_install(&host_config);
    ESP_LOGW(TAG, "usb_host_install result: %s (0x%x)", esp_err_to_name(ret), ret);
    if (ret != ESP_OK) {
        return ret;
    }
    ESP_LOGW(TAG, "=== USB host installed successfully ===");
    if (ret != ESP_OK)
        return ret;

    host_mode_active = true;
    if (xTaskCreate(usb_host_lib_task, "usb_host", 3072, NULL, 5, &usb_host_task_handle) != pdPASS) {
        usb_host_uninstall();
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGW(TAG, "=== Registering USB host client ===");
    const usb_host_client_config_t client_config = {
        .is_synchronous    = true,  // Use synchronous mode (no callback needed)
        .max_num_event_msg = 5,
        .async             = {.client_event_callback = NULL, .callback_arg = NULL},
    };

    ret = usb_host_client_register(&client_config, &client_handle);
    ESP_LOGW(TAG, "usb_host_client_register result: %s (0x%x)", esp_err_to_name(ret), ret);
    if (ret != ESP_OK) {
        host_mode_active = false;
        vTaskDelete(usb_host_task_handle);
        usb_host_uninstall();
        return ret;
    }

    ESP_LOGW(TAG, "=== Installing CDC ACM host driver ===");
    const cdc_acm_host_driver_config_t driver_config = {
        .driver_task_stack_size = 4096,
        .driver_task_priority   = 10,
        .xCoreID                = 0,
        .new_dev_cb             = new_dev_callback,
    };

    ret = cdc_acm_host_install(&driver_config);
    ESP_LOGW(TAG, "cdc_acm_host_install result: %s (0x%x)", esp_err_to_name(ret), ret);
    if (ret != ESP_OK) {
        usb_host_client_deregister(client_handle);
        host_mode_active = false;
        vTaskDelete(usb_host_task_handle);
        usb_host_uninstall();
        return ret;
    }

    ESP_LOGW(TAG, "=== switch_to_host_mode completed successfully ===");
    return ESP_OK;
}

static esp_err_t switch_to_device_mode(void)
{
    ESP_LOGW(TAG, "=== Switching to USB device mode ===");

    if (cdc_device) {
        ESP_LOGW(TAG, "Closing CDC device");
        cdc_acm_host_close(cdc_device);
        cdc_device = NULL;
    }

    ESP_LOGW(TAG, "Uninstalling CDC ACM host");
    cdc_acm_host_uninstall();

    if (client_handle) {
        ESP_LOGW(TAG, "Deregistering USB host client");
        usb_host_client_deregister(client_handle);
        client_handle = NULL;
    }

    host_mode_active = false;
    if (usb_host_task_handle) {
        usb_host_task_handle = NULL;
    }

    ESP_LOGW(TAG, "Uninstalling USB host");
    usb_host_uninstall();
    
    // Wait for USB PHY to be fully released
    ESP_LOGW(TAG, "Waiting 500ms for USB PHY release");
    vTaskDelay(pdMS_TO_TICKS(500));

    // esp_tinyusb 1.7.6+ API
    ESP_LOGW(TAG, "Reinstalling TinyUSB device mode");
    tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,  // Use default
        .string_descriptor = NULL,  // Use default
        .string_descriptor_count = 0,
        .external_phy = false,
        .configuration_descriptor = NULL  // Use default
    };

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "TinyUSB already installed or PHY still in use - this is expected in some cases");
        ret = ESP_OK;  // Treat as success since device mode will work eventually
    }
    ESP_LOGW(TAG, "tinyusb_driver_install result: %s (0x%x)", esp_err_to_name(ret), ret);
    ESP_LOGW(TAG, "=== USB device mode restored ===");
    return ret;
}

static void pairing_task(void *arg)
{
    uint32_t start_time = esp_timer_get_time() / 1000;
    while (!cdc_device && (esp_timer_get_time() / 1000 - start_time) < PAIRING_TIMEOUT_MS) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (!cdc_device) {
        pairing_active = false;
        if (result_callback)
            result_callback(false, 0, "No device");
        switch_to_device_mode();
        vTaskDelete(NULL);
        return;
    }

    general_config_t config;
    general_config_get(&config);

    lora_config_t lora_cfg;
    lora_get_config(&lora_cfg);

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Build JSON-RPC request
    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "name", config.device_name);

    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(params, "mac", mac_str);

    char key_str[65];
    for (int i = 0; i < 32; i++) {
        snprintf(key_str + i * 2, 3, "%02x", lora_cfg.aes_key[i]);
    }
    cJSON_AddStringToObject(params, "aes_key", key_str);

    cJSON *request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "jsonrpc", "2.0");
    cJSON_AddStringToObject(request, "method", "paired:pair");
    cJSON_AddItemToObject(request, "params", params);
    cJSON_AddNumberToObject(request, "id", 1);

    char *json_string = cJSON_PrintUnformatted(request);
    if (!json_string) {
        cJSON_Delete(request);
        pairing_active = false;
        if (result_callback)
            result_callback(false, 0, "JSON serialization failed");
        switch_to_device_mode();
        vTaskDelete(NULL);
        return;
    }
    
    size_t len = strlen(json_string);
    char *command = malloc(len + 2);
    if (!command) {
        free(json_string);
        cJSON_Delete(request);
        pairing_active = false;
        if (result_callback)
            result_callback(false, 0, "Memory allocation failed");
        switch_to_device_mode();
        vTaskDelete(NULL);
        return;
    }
    
    snprintf(command, len + 2, "%s\n", json_string);

    esp_err_t ret = cdc_acm_host_data_tx_blocking(cdc_device, (uint8_t *)command, strlen(command), 1000);
    free(command);
    free(json_string);
    cJSON_Delete(request);

    if (ret != ESP_OK) {
        pairing_active = false;
        if (result_callback)
            result_callback(false, 0, "Send failed");
        switch_to_device_mode();
        vTaskDelete(NULL);
        return;
    }

    start_time = esp_timer_get_time() / 1000;
    while (pairing_active && !pairing_success && (esp_timer_get_time() / 1000 - start_time) < PAIRING_TIMEOUT_MS) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    pairing_active = false;
    if (result_callback) {
        if (pairing_success) {
            result_callback(true, paired_device_id, paired_device_name);
        } else {
            result_callback(false, 0, "Timeout");
        }
    }

    switch_to_device_mode();
    vTaskDelete(NULL);
}

esp_err_t usb_pairing_start(usb_pairing_callback_t callback)
{
    if (pairing_active)
        return ESP_ERR_INVALID_STATE;

    result_callback  = callback;
    pairing_active   = true;
    pairing_success  = false;
    paired_device_id = 0;
    memset(paired_device_name, 0, sizeof(paired_device_name));

    esp_err_t ret = switch_to_host_mode();
    if (ret != ESP_OK) {
        pairing_active = false;
        return ret;
    }

    if (xTaskCreate(pairing_task, "usb_pairing", 3072, NULL, 5, NULL) != pdPASS) {
        switch_to_device_mode();
        pairing_active = false;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t usb_pairing_stop(void)
{
    ESP_LOGW(TAG, "=== usb_pairing_stop called ===");
    ESP_LOGW(TAG, "pairing_active=%d, host_mode_active=%d", pairing_active, host_mode_active);
    
    pairing_active = false;
    
    if (host_mode_active) {
        ESP_LOGW(TAG, "Host mode active, switching to device mode");
        switch_to_device_mode();
    } else {
        ESP_LOGW(TAG, "Host mode not active, skipping device mode switch");
    }

    ESP_LOGW(TAG, "=== usb_pairing_stop completed ===");
    return ESP_OK;
}
