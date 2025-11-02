#include "ui_mini.h"
#include "bsp.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "general_config.h"
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

// Encapsulated UI state (private to this module)
static ui_mini_status_t s_status = {0};
static SemaphoreHandle_t status_mutex = NULL;

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

    // Create status mutex
    status_mutex = xSemaphoreCreateMutex();
    if (!status_mutex) {
        ESP_LOGE(TAG, "Failed to create status mutex");
        vSemaphoreDelete(draw_mutex);
        return ESP_ERR_NO_MEM;
    }

    // Initialize status with defaults
    memset(&s_status, 0, sizeof(s_status));

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

static uint8_t s_ota_progress = 0;

esp_err_t ui_mini_show_ota_update(void)
{
    s_ota_progress = 0;
    return ui_mini_set_screen(OLED_SCREEN_OTA_UPDATE);
}

esp_err_t ui_mini_update_ota_progress(uint8_t progress)
{
    if (progress > 100) progress = 100;
    s_ota_progress = progress;
    return ESP_OK;
}

uint8_t ui_mini_get_ota_progress(void)
{
    return s_ota_progress;
}

ui_mini_status_t* ui_mini_get_status(void)
{
    if (!status_mutex) {
        return NULL;
    }
    
    xSemaphoreTake(status_mutex, portMAX_DELAY);
    
    // Merge runtime state with config data
    general_config_t config;
    general_config_get(&config);
    
    // Generate device_id from MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    s_status.device_id = (mac[4] << 8) | mac[5];
    
    strncpy(s_status.device_name, config.device_name, sizeof(s_status.device_name) - 1);
    
    xSemaphoreGive(status_mutex);
    
    return &s_status;
}

esp_err_t ui_mini_update_status(const ui_mini_status_t *status)
{
    if (!status_mutex || !status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xSemaphoreTake(status_mutex, portMAX_DELAY);
    
    // Update runtime state (don't overwrite device_id/name - they come from config)
    s_status.battery_level = status->battery_level;
    s_status.battery_charging = status->battery_charging;
    s_status.lora_connected = status->lora_connected;
    s_status.lora_signal = status->lora_signal;
    s_status.usb_connected = status->usb_connected;
    s_status.bluetooth_connected = status->bluetooth_connected;
    strncpy(s_status.last_command, status->last_command, sizeof(s_status.last_command) - 1);
    s_status.active_presenter_count = status->active_presenter_count;
    
    // Copy active presenters
    memcpy(s_status.active_presenters, status->active_presenters, 
           sizeof(s_status.active_presenters));
    
    // Copy command history
    memcpy(s_status.command_history, status->command_history,
           sizeof(s_status.command_history));
    s_status.command_history_count = status->command_history_count;
    
    xSemaphoreGive(status_mutex);
    
    return ESP_OK;
}

