#include "lv_port_indev.h"
#include "bsp.h"
#include "esp_log.h"

static const char *TAG = "lv_port_indev";

static void button_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static bool prev_pressed = false;
    static bool next_pressed = false;

    // Read button states
    bool prev_now = bsp_read_button(BSP_BUTTON_PREV);
    bool next_now = bsp_read_button(BSP_BUTTON_NEXT);

    // Simple button to key mapping
    if (prev_now && !prev_pressed) {
        data->key   = LV_KEY_LEFT;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (next_now && !next_pressed) {
        data->key   = LV_KEY_RIGHT;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    prev_pressed = prev_now;
    next_pressed = next_now;
}

lv_indev_t *lv_port_indev_init(void)
{
    // Create button input device
    lv_indev_t *indev = lv_indev_create();
    if (!indev) {
        ESP_LOGE(TAG, "Failed to create input device");
        return NULL;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, button_read_cb);

    ESP_LOGI(TAG, "LVGL input device initialized");
    return indev;
}

void lv_port_indev_deinit(lv_indev_t *indev)
{
    if (indev) {
        lv_indev_delete(indev);
    }
}
