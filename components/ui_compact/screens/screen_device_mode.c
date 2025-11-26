#include "lvgl.h"
#include "ui_components.h"
#include "general_config.h"
#include "system_events.h"
#include "esp_log.h"

static const char *TAG = "device_mode";
static ui_radio_select_t *radio = NULL;

static const char *mode_items[] = {"PRESENTER", "PC"};
#define MODE_COUNT 2

void screen_device_mode_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    if (!radio) {
        radio = ui_radio_select_create(MODE_COUNT, UI_RADIO_SINGLE);
        
        // Set initial selection based on config
        general_config_t config;
        general_config_get(&config);
        radio->selected_index = (config.device_mode == DEVICE_MODE_PRESENTER) ? 0 : 1;
    }
    
    ui_radio_select_render(radio, parent, "DEVICE MODE", mode_items);
}

void screen_device_mode_navigate_down(void) {
    ui_radio_select_navigate_down(radio);
}

void screen_device_mode_select(void) {
    if (!radio) return;
    
    general_config_t config;
    general_config_get(&config);
    
    device_mode_t new_mode = (radio->selected_index == 0) ? DEVICE_MODE_PRESENTER : DEVICE_MODE_PC;
    
    if (new_mode != config.device_mode) {
        config.device_mode = new_mode;
        general_config_set(&config);
        
        system_event_mode_t evt = {.mode = new_mode};
        esp_event_post_to(system_events_get_loop(), SYSTEM_EVENTS, SYSTEM_EVENT_MODE_CHANGED, &evt, sizeof(evt), 0);
        
        ESP_LOGI(TAG, "Device mode changed to: %s", new_mode == DEVICE_MODE_PRESENTER ? "PRESENTER" : "PC");
    }
}

void screen_device_mode_reset(void) {
    if (radio) {
        if (radio->selected_items) {
            free(radio->selected_items);
        }
        free(radio);
        radio = NULL;
    }
}
