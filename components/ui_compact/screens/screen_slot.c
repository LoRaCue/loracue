#include "lvgl.h"
#include "ui_components.h"
#include "general_config.h"
#include "esp_log.h"

static const char *TAG = "slot_screen";
static ui_edit_screen_t *screen = NULL;
static int selected_slot = 0;

void screen_slot_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    if (!screen) {
        screen = ui_edit_screen_create("SLOT");
    }
    
    char value_text[32];
    snprintf(value_text, sizeof(value_text), "Slot %d", selected_slot + 1);
    ui_edit_screen_render(screen, parent, "SLOT", value_text, selected_slot, 15);
}

void screen_slot_init(void) {
    general_config_t config;
    general_config_get(&config);
    selected_slot = config.slot_id - 1;
    if (screen) {
        screen->edit_mode = false;
    }
}

void screen_slot_navigate_down(void) {
    if (!screen || !screen->edit_mode) return;
    selected_slot = (selected_slot + 1) % 16;
}

void screen_slot_navigate_up(void) {
    if (!screen || !screen->edit_mode) return;
    selected_slot = (selected_slot - 1 + 16) % 16;
}

void screen_slot_select(void) {
    if (!screen) return;
    
    if (screen->edit_mode) {
        general_config_t config;
        general_config_get(&config);
        config.slot_id = selected_slot + 1;
        general_config_set(&config);
        ESP_LOGI(TAG, "Slot saved: %d", config.slot_id);
        screen->edit_mode = false;
    } else {
        screen->edit_mode = true;
    }
}

bool screen_slot_is_edit_mode(void) {
    return screen ? screen->edit_mode : false;
}
