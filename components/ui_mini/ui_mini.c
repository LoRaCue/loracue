#include "ui_mini.h"
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

static const char *TAG               = "ui_mini";
static bool ui_initialized           = false;
static SemaphoreHandle_t draw_mutex  = NULL;
static bool background_tasks_enabled = true;

// Global u8g2 instance (initialized by BSP)
extern u8g2_t u8g2;

void ui_mini_enable_background_tasks(bool enable)
{
    background_tasks_enabled = enable;
}

bool ui_mini_background_tasks_enabled(void)
{
    return background_tasks_enabled;
}

bool ui_mini_try_lock_draw(void)
{
    if (!draw_mutex)
        return false;
    return xSemaphoreTake(draw_mutex, 0) == pdTRUE;
}

void ui_mini_unlock_draw(void)
{
    if (draw_mutex) {
        xSemaphoreGive(draw_mutex);
    }
}

esp_err_t ui_mini_display_off(void)
{
    if (!ui_mini_try_lock_draw()) {
        return ESP_ERR_TIMEOUT;
    }
    u8g2_SetPowerSave(&u8g2, 1);
    ui_mini_unlock_draw();
    return ESP_OK;
}

esp_err_t ui_mini_display_on(void)
{
    if (!ui_mini_try_lock_draw()) {
        return ESP_ERR_TIMEOUT;
    }
    u8g2_SetPowerSave(&u8g2, 0);
    ui_mini_unlock_draw();
    return ESP_OK;
}

esp_err_t ui_mini_init(void)
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

esp_err_t ui_mini_set_screen(ui_mini_screen_t screen)
{
    ui_screen_controller_set(screen, NULL);
    return ESP_OK;
}

ui_mini_screen_t ui_mini_get_screen(void)
{
    return ui_screen_controller_get_current();
}

esp_err_t ui_mini_show_message(const char *title, const char *message, uint32_t timeout_ms)
{
    ESP_LOGI(TAG, "Message: %s - %s", title, message);
    // TODO: Implement message overlay screen
    return ESP_OK;
}

esp_err_t ui_mini_update_status(const ui_mini_status_t *status)
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

static uint8_t g_ota_progress = 0;

esp_err_t ui_mini_show_ota_update(void)
{
    g_ota_progress = 0;
    return ui_mini_set_screen(OLED_SCREEN_OTA_UPDATE);
}

esp_err_t ui_mini_update_ota_progress(uint8_t progress)
{
    if (progress > 100) progress = 100;
    g_ota_progress = progress;
    
    // Force immediate redraw
    xSemaphoreGive(g_ui_update_sem);
    return ESP_OK;
}

uint8_t ui_mini_get_ota_progress(void)
{
    return g_ota_progress;
}
