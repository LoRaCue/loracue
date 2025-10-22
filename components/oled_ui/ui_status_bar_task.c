#include "ui_status_bar_task.h"
#include "bluetooth_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_data_provider.h"
#include "ui_screen_controller.h"
#include "ui_status_bar.h"

static const char *TAG                     = "ui_status_bar";
static TaskHandle_t status_bar_task_handle = NULL;
static bool task_running                   = false;

static void ui_status_bar_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Status bar update task started");

    TickType_t last_update = xTaskGetTickCount();

    while (task_running) {
        // Check if background tasks are enabled
        extern bool oled_ui_background_tasks_enabled(void);
        if (!oled_ui_background_tasks_enabled()) {
            vTaskDelayUntil(&last_update, pdMS_TO_TICKS(5000));
            continue;
        }

        oled_screen_t current = ui_screen_controller_get_current();

        // Only update MAIN screen (which adapts to mode)
        if (current == OLED_SCREEN_MAIN) {
            // Try to acquire draw lock
            extern bool oled_ui_try_lock_draw(void);
            extern void oled_ui_unlock_draw(void);

            if (oled_ui_try_lock_draw()) {
                const ui_status_t *status = ui_data_provider_get_status();
                if (status) {
                    ui_screen_controller_update(status);
                }
                oled_ui_unlock_draw();
            }
        }

        // Dynamic update interval: 500ms for low battery or Bluetooth pairing, 5s otherwise
        const ui_status_t *status = ui_data_provider_get_status();
        bool needs_fast_update    = (status && status->battery_level <= 5);

        // Also use fast updates during Bluetooth pairing
        uint32_t dummy_passkey;
        if (bluetooth_config_get_passkey(&dummy_passkey)) {
            needs_fast_update = true;
        }

        TickType_t update_interval = needs_fast_update ? pdMS_TO_TICKS(500)   // Fast updates for blinking/pairing
                                                       : pdMS_TO_TICKS(5000); // Normal updates

        // Wait for next update interval
        vTaskDelayUntil(&last_update, update_interval);
    }

    ESP_LOGI(TAG, "Status bar update task stopped");
    vTaskDelete(NULL);
}

esp_err_t ui_status_bar_task_start(void)
{
    if (task_running) {
        ESP_LOGW(TAG, "Status bar task already running");
        return ESP_ERR_INVALID_STATE;
    }

    task_running = true;

    BaseType_t ret = xTaskCreate(ui_status_bar_task, "ui_status", 4096, NULL,
                                 5, // Normal priority
                                 &status_bar_task_handle);

    if (ret != pdPASS) {
        task_running = false;
        ESP_LOGE(TAG, "Failed to create status bar task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Status bar task started successfully");
    return ESP_OK;
}

esp_err_t ui_status_bar_task_stop(void)
{
    if (!task_running) {
        return ESP_ERR_INVALID_STATE;
    }

    task_running = false;
    return ESP_OK;
}
