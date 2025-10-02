#include "lora_comm.h"
#include "lora_protocol.h"
#include "button_manager.h"
#include "device_config.h"
#include "usb_hid.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LORA_COMM";
static TaskHandle_t lora_task_handle = NULL;
static bool lora_task_running = false;

static void handle_lora_command(lora_command_t command)
{
    ESP_LOGI(TAG, "Received LoRa command: 0x%02X", command);
    
    usb_hid_keycode_t keycode;
    
    switch (command) {
        case CMD_NEXT_SLIDE:
            keycode = HID_KEY_PAGE_DOWN;
            break;
        case CMD_PREV_SLIDE:
            keycode = HID_KEY_PAGE_UP;
            break;
        case CMD_BLACK_SCREEN:
            keycode = HID_KEY_B;
            break;
        case CMD_START_PRESENTATION:
            keycode = HID_KEY_F5;
            break;
        default:
            ESP_LOGW(TAG, "Unknown LoRa command: 0x%02X", command);
            return;
    }
    
    usb_hid_send_key(keycode);
}

static void lora_receive_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LoRa receive task started");
    
    while (lora_task_running) {
        lora_packet_data_t packet_data;
        esp_err_t ret = lora_protocol_receive_packet(&packet_data, 1000);
        
        if (ret == ESP_OK) {
            handle_lora_command(packet_data.command);
        } else if (ret != ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "LoRa receive error: %s", esp_err_to_name(ret));
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    ESP_LOGI(TAG, "LoRa receive task stopped");
    vTaskDelete(NULL);
}

static void lora_button_handler(button_event_type_t event, void* arg)
{
    device_config_t config;
    device_config_get(&config);
    
    lora_command_t lora_cmd;
    switch (event) {
        case BUTTON_EVENT_PREV_SHORT:
            lora_cmd = CMD_PREV_SLIDE;
            break;
        case BUTTON_EVENT_NEXT_SHORT:
            lora_cmd = CMD_NEXT_SLIDE;
            break;
        case BUTTON_EVENT_PREV_LONG:
            lora_cmd = CMD_BLACK_SCREEN;
            break;
        case BUTTON_EVENT_NEXT_LONG:
            lora_cmd = CMD_START_PRESENTATION;
            break;
        default:
            return;
    }
    
    esp_err_t ret = lora_protocol_send_command(lora_cmd, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send LoRa command: %s", esp_err_to_name(ret));
    }
}

esp_err_t lora_comm_init(void)
{
    ESP_LOGI(TAG, "Initializing LoRa communication");
    
    ESP_ERROR_CHECK(button_manager_register_callback(lora_button_handler, NULL));
    
    ESP_LOGI(TAG, "LoRa communication initialized");
    return ESP_OK;
}

esp_err_t lora_comm_start(void)
{
    ESP_LOGI(TAG, "Starting LoRa communication task");
    
    lora_task_running = true;
    BaseType_t ret = xTaskCreate(
        lora_receive_task,
        "lora_rx",
        4096,
        NULL,
        4,
        &lora_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LoRa receive task");
        lora_task_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "LoRa communication started");
    return ESP_OK;
}
