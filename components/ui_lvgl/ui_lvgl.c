#include "ui_lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ui_lvgl";

static lv_display_t *display = NULL;
static lv_indev_t *indev = NULL;
static esp_timer_handle_t lvgl_tick_timer = NULL;
static TaskHandle_t lvgl_task_handle = NULL;

void ui_lvgl_tick_cb(void *arg) {
    lv_tick_inc(1);
}

static void lvgl_task(void *arg) {
    ESP_LOGI(TAG, "LVGL task started");
    
    while (1) {
        uint32_t time_till_next = lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(time_till_next));
    }
}

esp_err_t ui_lvgl_init(void) {
    ESP_LOGI(TAG, "Initializing LVGL UI system");

    // Initialize LVGL
    lv_init();

    // Initialize display
    display = lv_port_disp_init();
    if (!display) {
        ESP_LOGE(TAG, "Failed to initialize display");
        return ESP_FAIL;
    }

    // Initialize input device
    indev = lv_port_indev_init();
    if (!indev) {
        ESP_LOGE(TAG, "Failed to initialize input device");
        lv_port_disp_deinit();
        return ESP_FAIL;
    }

    // Create tick timer (1ms)
    const esp_timer_create_args_t timer_args = {
        .callback = ui_lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    
    esp_err_t ret = esp_timer_create(&timer_args, &lvgl_tick_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create tick timer");
        return ret;
    }
    
    esp_timer_start_periodic(lvgl_tick_timer, 1000); // 1ms

    // Create LVGL task
    xTaskCreate(lvgl_task, "lvgl", 4096, NULL, 5, &lvgl_task_handle);

    ESP_LOGI(TAG, "LVGL UI system initialized");
    return ESP_OK;
}

esp_err_t ui_lvgl_deinit(void) {
    if (lvgl_task_handle) {
        vTaskDelete(lvgl_task_handle);
        lvgl_task_handle = NULL;
    }

    if (lvgl_tick_timer) {
        esp_timer_stop(lvgl_tick_timer);
        esp_timer_delete(lvgl_tick_timer);
        lvgl_tick_timer = NULL;
    }

    lv_port_indev_deinit(indev);
    lv_port_disp_deinit();
    
    lv_deinit();

    return ESP_OK;
}

lv_display_t *ui_lvgl_get_display(void) {
    return display;
}
