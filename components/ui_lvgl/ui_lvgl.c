#include "ui_lvgl.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

static const char *TAG = "ui_lvgl";

static lv_display_t *display = NULL;
static lv_indev_t *indev     = NULL;

esp_err_t ui_lvgl_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL core");

    // Initialize LVGL port
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // Initialize display
    display = lv_port_disp_init();
    if (display == NULL) {
        ESP_LOGE(TAG, "Failed to initialize display");
        return ESP_FAIL;
    }

    // Initialize input device
    indev = lv_port_indev_init();

    ESP_LOGI(TAG, "LVGL core initialized");
    return ESP_OK;
}

esp_err_t ui_lvgl_deinit(void)
{
    lvgl_port_deinit();
    return ESP_OK;
}

void ui_lvgl_lock(void)
{
    lvgl_port_lock(0);
}

void ui_lvgl_unlock(void)
{
    lvgl_port_unlock();
}

lv_display_t *ui_lvgl_get_display(void)
{
    return display;
}

void ui_lvgl_set_display(lv_display_t *disp)
{
    display = disp;
}

lv_indev_t *ui_lvgl_get_indev(void)
{
    return indev;
}

void ui_lvgl_set_indev(lv_indev_t *dev)
{
    indev = dev;
}
