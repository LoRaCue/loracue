#include "lora_comm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora_driver.h"
#include "lora_protocol.h"

static const char *TAG = "LORA_COMM";

static TaskHandle_t lora_task_handle = NULL;
static bool lora_task_running        = false;

static lora_comm_rx_callback_t rx_callback = NULL;
static void *rx_callback_ctx               = NULL;

static lora_comm_state_callback_t state_callback = NULL;
static void *state_callback_ctx                  = NULL;

static lora_connection_state_t last_connection_state = LORA_CONNECTION_LOST;

static void lora_receive_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LoRa receive task started");
    uint32_t consecutive_errors = 0;

    while (lora_task_running) {
        lora_packet_data_t packet_data;
        esp_err_t ret = lora_protocol_receive_packet(&packet_data, 1000);

        if (ret == ESP_OK) {
            consecutive_errors = 0;

            if (rx_callback) {
                int16_t rssi = lora_protocol_get_last_rssi();
                rx_callback(packet_data.device_id, packet_data.command, packet_data.payload, packet_data.payload_length,
                            rssi, rx_callback_ctx);
            }

            lora_connection_state_t state = lora_protocol_get_connection_state();
            if (state != last_connection_state) {
                last_connection_state = state;
                if (state_callback) {
                    state_callback(state, state_callback_ctx);
                }
            }
        } else if (ret != ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "LoRa receive error: %s", esp_err_to_name(ret));
            consecutive_errors++;

            if (consecutive_errors > 10) {
                ESP_LOGE(TAG, "LoRa connection lost, attempting recovery...");
                lora_connection_state_t state = lora_protocol_get_connection_state();

                if (state == LORA_CONNECTION_LOST && state != last_connection_state) {
                    last_connection_state = state;
                    if (state_callback) {
                        state_callback(state, state_callback_ctx);
                    }

                    ESP_LOGI(TAG, "Reinitializing LoRa driver");
                    lora_driver_init();
                    consecutive_errors = 0;
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGI(TAG, "LoRa receive task stopped");
    vTaskDelete(NULL);
}

esp_err_t lora_comm_init(void)
{
    ESP_LOGI(TAG, "Initializing LoRa communication");
    return ESP_OK;
}

esp_err_t lora_comm_register_rx_callback(lora_comm_rx_callback_t callback, void *user_ctx)
{
    rx_callback     = callback;
    rx_callback_ctx = user_ctx;
    return ESP_OK;
}

esp_err_t lora_comm_register_state_callback(lora_comm_state_callback_t callback, void *user_ctx)
{
    state_callback     = callback;
    state_callback_ctx = user_ctx;
    return ESP_OK;
}

esp_err_t lora_comm_send_command(lora_command_t command, const uint8_t *payload, uint8_t payload_length)
{
    return lora_protocol_send_command(command, payload, payload_length);
}

esp_err_t lora_comm_send_command_reliable(lora_command_t command, const uint8_t *payload, uint8_t payload_length)
{
    return lora_protocol_send_reliable(command, payload, payload_length, 1000, 2);
}

esp_err_t lora_comm_start(void)
{
    ESP_LOGI(TAG, "Starting LoRa communication task");

    esp_err_t rssi_ret = lora_protocol_start_rssi_monitor();
    if (rssi_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to start RSSI monitor: %s", esp_err_to_name(rssi_ret));
    }

    lora_task_running = true;
    BaseType_t ret    = xTaskCreate(lora_receive_task, "lora_rx", 4096, NULL, 4, &lora_task_handle);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LoRa receive task");
        lora_task_running = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LoRa communication started");
    return ESP_OK;
}

esp_err_t lora_comm_stop(void)
{
    if (!lora_task_running) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping LoRa communication task");
    lora_task_running = false;

    if (lora_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        lora_task_handle = NULL;
    }

    return ESP_OK;
}
