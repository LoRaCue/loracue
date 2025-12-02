#include "ui_navigator.h"
#include "esp_log.h"
#include "input_manager.h"
#include "ui_lvgl.h"
#include <string.h>

static const char *TAG = "ui_navigator";

#define MAX_SCREENS 32

static const ui_screen_t *screen_registry[MAX_SCREENS] = {0};
static const ui_screen_t *current_screen_impl          = NULL;
static lv_obj_t *current_lv_screen_obj                 = NULL;
static ui_screen_type_t current_type = (ui_screen_type_t)-1; // Default changed from UI_SCREEN_BOOT to -1
static ui_navigator_screen_change_cb_t screen_change_cb = NULL;

esp_err_t ui_navigator_init(void)
{
    ESP_LOGI(TAG, "Initializing UI Navigator");
    memset(screen_registry, 0, sizeof(screen_registry));
    current_screen_impl   = NULL;
    current_lv_screen_obj = NULL;
    screen_change_cb      = NULL; // Initialize callback
    return ESP_OK;
}

void ui_navigator_set_screen_change_callback(ui_navigator_screen_change_cb_t cb)
{
    screen_change_cb = cb;
}

esp_err_t ui_navigator_register_screen(ui_screen_type_t type, const ui_screen_t *screen_impl)
{
    if (type >= MAX_SCREENS) {
        ESP_LOGE(TAG, "Screen type %d out of range (max %d)", type, MAX_SCREENS);
        return ESP_ERR_INVALID_ARG;
    }
    if (screen_impl == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    screen_registry[type] = screen_impl;
    ESP_LOGD(TAG, "Registered screen type %d", type);
    return ESP_OK;
}

esp_err_t ui_navigator_switch_to(ui_screen_type_t screen_type)
{
    if (screen_type >= MAX_SCREENS || screen_registry[screen_type] == NULL) {
        ESP_LOGE(TAG, "Screen type %d not registered", screen_type);
        return ESP_ERR_NOT_FOUND;
    }

    ui_lvgl_lock();

    const ui_screen_t *next_screen = screen_registry[screen_type];

    // 1. Cleanup current screen
    if (current_screen_impl) {
        if (current_screen_impl->on_exit) {
            current_screen_impl->on_exit();
        }
        if (current_screen_impl->destroy) {
            current_screen_impl->destroy();
        }
    }

    // Note: We delete the LVGL object after creating the new one to avoid flicker,
    // or we can delete it now. Standard practice is often load new, then delete old.
    // However, for memory constrained devices, delete old first might be safer.
    // Let's stick to the pattern seen in ui_compact.c: create new, load, delete old.

    lv_obj_t *old_lv_screen = current_lv_screen_obj;

    // 2. Create new screen
    lv_obj_t *new_lv_screen = lv_obj_create(NULL);
    if (new_lv_screen == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL screen object");
        ui_lvgl_unlock();
        return ESP_ERR_NO_MEM;
    }

    if (next_screen->create) {
        next_screen->create(new_lv_screen);
    }

    // 3. Load new screen
    lv_screen_load(new_lv_screen);

    // 4. Delete old screen object
    if (old_lv_screen) {
        lv_obj_del(old_lv_screen);
    }

    // 5. Update state
    current_lv_screen_obj = new_lv_screen;
    current_screen_impl   = next_screen;
    current_type          = screen_type;

    if (next_screen->on_enter) {
        next_screen->on_enter();
    }

    if (screen_change_cb) {
        screen_change_cb(screen_type);
    }

    ui_lvgl_unlock();

    ESP_LOGI(TAG, "Switched to screen type %d", screen_type);

    return ESP_OK;
}

ui_screen_type_t ui_navigator_get_current_screen_type(void)
{
    return current_type;
}

void ui_navigator_handle_input_event(input_event_t event)
{
    if (current_screen_impl && current_screen_impl->handle_input_event) {
        current_screen_impl->handle_input_event(event);
    } else {
        ESP_LOGD(TAG, "No input event handler for current screen");
    }
}
