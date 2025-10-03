#include "ui_data_update_task.h"
#include "ui_data_provider.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "ui_data_update";
static TaskHandle_t data_task_handle = NULL;
static bool task_running = false;

static void ui_data_update_task(void* pvParameters) {
    ESP_LOGI(TAG, "Data update task started");
    
    TickType_t last_update = xTaskGetTickCount();
    const TickType_t update_interval = pdMS_TO_TICKS(5000);  // Update every 5 seconds
    
    while (task_running) {
        // Update data from BSP sensors (battery, USB, LoRa)
        esp_err_t ret = ui_data_provider_update();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to update data provider: %s", esp_err_to_name(ret));
        }
        
        // Wait for next update interval
        vTaskDelayUntil(&last_update, update_interval);
    }
    
    ESP_LOGI(TAG, "Data update task stopped");
    vTaskDelete(NULL);
}

esp_err_t ui_data_update_task_start(void) {
    if (task_running) {
        ESP_LOGW(TAG, "Data update task already running");
        return ESP_ERR_INVALID_STATE;
    }
    
    task_running = true;
    
    BaseType_t ret = xTaskCreate(
        ui_data_update_task,
        "ui_data_upd",
        3072,
        NULL,
        4,  // Lower priority than rendering
        &data_task_handle
    );
    
    if (ret != pdPASS) {
        task_running = false;
        ESP_LOGE(TAG, "Failed to create data update task");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Data update task started successfully");
    return ESP_OK;
}

esp_err_t ui_data_update_task_stop(void) {
    if (!task_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    task_running = false;
    return ESP_OK;
}
