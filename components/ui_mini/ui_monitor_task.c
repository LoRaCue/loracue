#include "ui_monitor_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_data_provider.h"
#include "ui_screen_controller.h"

static const char *TAG                  = "ui_monitor";
static TaskHandle_t monitor_task_handle = NULL;
static bool monitor_running             = false;

static void ui_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "UI monitor task started");

    TickType_t last_update           = xTaskGetTickCount();
    const TickType_t update_interval = pdMS_TO_TICKS(5000); // Update every 5 seconds

    while (monitor_running) {
        // Update data from BSP sensors
        esp_err_t ret = ui_data_provider_update();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to update data provider: %s", esp_err_to_name(ret));
        }
        // NOTE: Screen update is handled by ui_status_bar_task to avoid race conditions
        // Do not call ui_screen_controller_update() here

        // Wait for next update interval
        vTaskDelayUntil(&last_update, update_interval);
    }

    ESP_LOGI(TAG, "UI monitor task stopped");
    vTaskDelete(NULL);
}

esp_err_t ui_monitor_task_start(void)
{
    if (monitor_running) {
        ESP_LOGW(TAG, "Monitor task already running");
        return ESP_ERR_INVALID_STATE;
    }

    monitor_running = true;

    BaseType_t ret = xTaskCreate(ui_monitor_task, "ui_monitor", 3072, NULL,
                                 5, // Priority
                                 &monitor_task_handle);

    if (ret != pdPASS) {
        monitor_running = false;
        ESP_LOGE(TAG, "Failed to create monitor task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "UI monitor task started successfully");
    return ESP_OK;
}

esp_err_t ui_monitor_task_stop(void)
{
    if (!monitor_running) {
        ESP_LOGW(TAG, "Monitor task not running");
        return ESP_ERR_INVALID_STATE;
    }

    monitor_running = false;

    // Task will delete itself
    if (monitor_task_handle) {
        monitor_task_handle = NULL;
    }

    ESP_LOGI(TAG, "UI monitor task stop requested");
    return ESP_OK;
}
