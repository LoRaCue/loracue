/**
 * @file usb_pairing.c
 * @brief USB-based device pairing with mode switching
 */

#include "usb_pairing.h"
#include "cJSON.h"
#include "device_config.h"
#include "device_registry.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "usb/usb_host.h"
#include "usb/cdc_acm_host.h"
#include <string.h>

static const char *TAG = "usb_pairing";

static usb_pairing_callback_t result_callback = NULL;
static bool pairing_active = false;
static bool pairing_success = false;
static uint16_t paired_device_id = 0;
static char paired_device_name[32] = {0};
static cdc_acm_dev_hdl_t cdc_device = NULL;
static usb_host_client_handle_t client_handle = NULL;
static TaskHandle_t usb_host_task_handle = NULL;
static bool host_mode_active = false;

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

static void cdc_rx_callback(uint8_t *data, size_t data_len, void *user_arg)
{
    if (!pairing_active) return;

    char response[256];
    size_t copy_len = (data_len < sizeof(response) - 1) ? data_len : sizeof(response) - 1;
    memcpy(response, data, copy_len);
    response[copy_len] = '\0';

    ESP_LOGI(TAG, "Received: %s", response);

    if (strncmp(response, "OK ", 3) == 0) {
        pairing_success = true;
        char *name_start = strstr(response, "with ");
        if (name_start) {
            name_start += 5;
            char *id_start = strstr(name_start, " (ID: ");
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
        .out_buffer_size = 512,
        .event_cb = NULL,
        .data_cb = cdc_rx_callback,
        .user_arg = NULL,
    };

    if (cdc_acm_host_open(ESP32_VID, ESP32_PID, 0, &dev_config, &cdc_device) == ESP_OK) {
        ESP_LOGI(TAG, "CDC-ACM opened");
    }
}

static esp_err_t switch_to_host_mode(void)
{
    ESP_LOGI(TAG, "Switching to USB host mode");
    vTaskDelay(pdMS_TO_TICKS(100));

    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) return ret;

    host_mode_active = true;
    if (xTaskCreate(usb_host_lib_task, "usb_host", 4096, NULL, 5, &usb_host_task_handle) != pdPASS) {
        usb_host_uninstall();
        return ESP_ERR_NO_MEM;
    }

    const usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {.client_event_callback = NULL, .callback_arg = NULL},
    };

    ret = usb_host_client_register(&client_config, &client_handle);
    if (ret != ESP_OK) {
        host_mode_active = false;
        vTaskDelete(usb_host_task_handle);
        usb_host_uninstall();
        return ret;
    }

    const cdc_acm_host_driver_config_t driver_config = {
        .driver_task_stack_size = 4096,
        .driver_task_priority = 10,
        .xCoreID = 0,
        .new_dev_cb = new_dev_callback,
    };

    ret = cdc_acm_host_install(&driver_config);
    if (ret != ESP_OK) {
        usb_host_client_deregister(client_handle);
        host_mode_active = false;
        vTaskDelete(usb_host_task_handle);
        usb_host_uninstall();
        return ret;
    }

    return ESP_OK;
}

static esp_err_t switch_to_device_mode(void)
{
    ESP_LOGI(TAG, "Switching to USB device mode");

    if (cdc_device) {
        cdc_acm_host_close(cdc_device);
        cdc_device = NULL;
    }

    cdc_acm_host_uninstall();

    if (client_handle) {
        usb_host_client_deregister(client_handle);
        client_handle = NULL;
    }

    host_mode_active = false;
    if (usb_host_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        usb_host_task_handle = NULL;
    }

    usb_host_uninstall();
    vTaskDelay(pdMS_TO_TICKS(200));

    tinyusb_config_t tusb_cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = {.skip_setup = false, .self_powered = false},
        .task = {.size = 4096, .priority = 5, .xCoreID = 0},
        .descriptor = {0},
        .event_cb = NULL,
        .event_arg = NULL
    };

    return tinyusb_driver_install(&tusb_cfg);
}

static void pairing_task(void *arg)
{
    uint32_t start_time = esp_timer_get_time() / 1000;
    while (!cdc_device && (esp_timer_get_time() / 1000 - start_time) < PAIRING_TIMEOUT_MS) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (!cdc_device) {
        pairing_active = false;
        if (result_callback) result_callback(false, 0, "No device");
        switch_to_device_mode();
        vTaskDelete(NULL);
        return;
    }

    device_config_t config;
    device_config_get(&config);

    uint8_t mac[6], aes_key[32];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    esp_fill_random(aes_key, sizeof(aes_key));

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "name", config.device_name);

    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(json, "mac", mac_str);

    char key_str[65];
    for (int i = 0; i < 32; i++) {
        snprintf(key_str + i * 2, 3, "%02x", aes_key[i]);
    }
    cJSON_AddStringToObject(json, "key", key_str);

    char *json_string = cJSON_Print(json);
    char command[512];
    snprintf(command, sizeof(command), "PAIR_DEVICE %s\n", json_string);

    esp_err_t ret = cdc_acm_host_data_tx_blocking(cdc_device, (uint8_t *)command, strlen(command), 1000);
    free(json_string);
    cJSON_Delete(json);

    if (ret != ESP_OK) {
        pairing_active = false;
        if (result_callback) result_callback(false, 0, "Send failed");
        switch_to_device_mode();
        vTaskDelete(NULL);
        return;
    }

    start_time = esp_timer_get_time() / 1000;
    while (pairing_active && !pairing_success && (esp_timer_get_time() / 1000 - start_time) < PAIRING_TIMEOUT_MS) {
        vTaskDelay(pdMS_TO_TICKS(100));
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
    if (pairing_active) return ESP_ERR_INVALID_STATE;

    result_callback = callback;
    pairing_active = true;
    pairing_success = false;
    paired_device_id = 0;
    memset(paired_device_name, 0, sizeof(paired_device_name));

    esp_err_t ret = switch_to_host_mode();
    if (ret != ESP_OK) {
        pairing_active = false;
        return ret;
    }

    if (xTaskCreate(pairing_task, "usb_pairing", 4096, NULL, 5, NULL) != pdPASS) {
        switch_to_device_mode();
        pairing_active = false;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t usb_pairing_stop(void)
{
    if (!pairing_active) return ESP_OK;

    pairing_active = false;
    if (host_mode_active) {
        switch_to_device_mode();
    }

    return ESP_OK;
}
