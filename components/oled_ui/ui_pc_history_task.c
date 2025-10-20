#include "ui_pc_history_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "general_config.h"
#include "pc_mode_screen.h"
#include "ui_screen_controller.h"

static const char *TAG                     = "ui_pc_history";
static TaskHandle_t pc_history_task_handle = NULL;
static bool task_running                   = false;

static void ui_pc_history_task(void *pvParameters)
{
    ESP_LOGI(TAG, "PC mode history task started");

    // Wait 10 seconds before first update
    vTaskDelay(pdMS_TO_TICKS(10000));

    TickType_t last_update           = xTaskGetTickCount();
    const TickType_t update_interval = pdMS_TO_TICKS(1000); // Update every 1 second

    while (task_running) {
        // Check if background tasks are enabled
        extern bool oled_ui_background_tasks_enabled(void);
        if (!oled_ui_background_tasks_enabled()) {
            vTaskDelayUntil(&last_update, update_interval);
            continue;
        }

        oled_screen_t current = ui_screen_controller_get_current();

        // Only update when on MAIN screen in PC mode
        if (current == OLED_SCREEN_MAIN) {
            // Check if we're in PC mode
            extern device_mode_t current_device_mode;

            if (current_device_mode == DEVICE_MODE_PC) {
                // Try to acquire draw lock
                extern bool oled_ui_try_lock_draw(void);
                extern void oled_ui_unlock_draw(void);

                if (oled_ui_try_lock_draw()) {
                    extern oled_status_t g_oled_status;
                    pc_mode_screen_draw(&g_oled_status);
                    oled_ui_unlock_draw();
                }
            }
        }

        // Wait for next update interval
        vTaskDelayUntil(&last_update, update_interval);
    }

    ESP_LOGI(TAG, "PC mode history task stopped");
    vTaskDelete(NULL);
}

esp_err_t ui_pc_history_task_start(void)
{
    if (task_running) {
        ESP_LOGW(TAG, "PC history task already running");
        return ESP_ERR_INVALID_STATE;
    }

    task_running = true;

    BaseType_t ret = xTaskCreate(ui_pc_history_task, "ui_pc_hist", 4096, NULL,
                                 5, // Normal priority
                                 &pc_history_task_handle);

    if (ret != pdPASS) {
        task_running = false;
        ESP_LOGE(TAG, "Failed to create PC history task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "PC history task started successfully");
    return ESP_OK;
}

esp_err_t ui_pc_history_task_stop(void)
{
    if (!task_running) {
        return ESP_ERR_INVALID_STATE;
    }

    task_running = false;
    return ESP_OK;
}
