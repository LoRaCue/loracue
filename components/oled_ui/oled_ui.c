#include "oled_ui.h"
#include "bsp.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "u8g2.h"
#include "ui_data_provider.h"
#include "ui_data_update_task.h"
#include "ui_pc_history_task.h"
#include "ui_screen_controller.h"
#include "ui_status_bar_task.h"
#include "version.h"
#include <string.h>

static const char *TAG               = "oled_ui";
static bool ui_initialized           = false;
static SemaphoreHandle_t draw_mutex  = NULL;
static bool background_tasks_enabled = true;

// Global u8g2 instance (initialized by BSP)
extern u8g2_t u8g2;

void oled_ui_enable_background_tasks(bool enable)
{
    background_tasks_enabled = enable;
}

bool oled_ui_background_tasks_enabled(void)
{
    return background_tasks_enabled;
}

bool oled_ui_try_lock_draw(void)
{
    if (!draw_mutex)
        return false;
    return xSemaphoreTake(draw_mutex, 0) == pdTRUE;
}

void oled_ui_unlock_draw(void)
{
    if (draw_mutex) {
        xSemaphoreGive(draw_mutex);
    }
}

esp_err_t oled_ui_display_off(void)
{
    if (!oled_ui_try_lock_draw()) {
        return ESP_ERR_TIMEOUT;
    }
    u8g2_SetPowerSave(&u8g2, 1);
    oled_ui_unlock_draw();
    return ESP_OK;
}

esp_err_t oled_ui_display_on(void)
{
    if (!oled_ui_try_lock_draw()) {
        return ESP_ERR_TIMEOUT;
    }
    u8g2_SetPowerSave(&u8g2, 0);
    oled_ui_unlock_draw();
    return ESP_OK;
}

esp_err_t oled_ui_init(void)
{
    ESP_LOGI(TAG, "Initializing OLED UI");

    // Create draw mutex
    draw_mutex = xSemaphoreCreateMutex();
    if (!draw_mutex) {
        ESP_LOGE(TAG, "Failed to create draw mutex");
        return ESP_ERR_NO_MEM;
    }

    // u8g2 is already initialized by BSP
    // Just verify it's ready
    u8g2_ClearDisplay(&u8g2);

    // Initialize screen controller
    ui_screen_controller_init();

    // Start three specialized tasks
    esp_err_t ret;

    // Task 1: Data provider updates (sensors only, no drawing)
    ret = ui_data_update_task_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start data update task: %s", esp_err_to_name(ret));
        return ret;
    }

    // Task 2: Status bar updates (USB, RF, battery icons)
    ret = ui_status_bar_task_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start status bar task: %s", esp_err_to_name(ret));
        return ret;
    }

    // Task 3: PC mode command history updates
    ret = ui_pc_history_task_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start PC history task: %s", esp_err_to_name(ret));
        return ret;
    }

    ui_initialized = true;
    ESP_LOGI(TAG, "OLED UI initialized successfully");
    return ESP_OK;
}

esp_err_t oled_ui_set_screen(oled_screen_t screen)
{
    ui_screen_controller_set(screen, NULL);
    return ESP_OK;
}

oled_screen_t oled_ui_get_screen(void)
{
    return ui_screen_controller_get_current();
}

esp_err_t oled_ui_show_message(const char *title, const char *message, uint32_t timeout_ms)
{
    ESP_LOGI(TAG, "Message: %s - %s", title, message);
    // TODO: Implement message overlay screen
    return ESP_OK;
}

esp_err_t oled_ui_update_status(const oled_status_t *status)
{
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }

    // Ignore updates before UI is fully initialized
    if (!ui_initialized) {
        return ESP_OK;
    }

    // Update data provider with new values
    esp_err_t ret = ui_data_provider_force_update(status->usb_connected, status->lora_connected, status->battery_level);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to update status: %s", esp_err_to_name(ret));
        return ret;
    }

    // Screen will be updated by monitor task periodically
    // No need to redraw here to avoid conflicts
    return ESP_OK;
}
