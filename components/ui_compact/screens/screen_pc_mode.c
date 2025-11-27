#include "lvgl.h"
#include "ui_compact_statusbar.h"
#include "ui_compact_fonts.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "system_events.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

static const char *TAG = "screen_pc_mode";
static lv_obj_t *statusbar = NULL;
static lv_obj_t *mode_label = NULL;
static lv_obj_t *waiting_label1 = NULL;
static lv_obj_t *waiting_label2 = NULL;

static struct {
    command_history_entry_t history[4];
    uint8_t count;
    uint8_t lightbar_state;
    uint32_t last_timestamp;
} screen_state = {0};

LV_IMG_DECLARE(button_long_press);

static void hid_command_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    const system_event_hid_command_t *evt = (const system_event_hid_command_t *)data;
    
    uint8_t modifiers = 0, keycode = 0;
    system_event_get_keyboard_data(evt, &modifiers, &keycode);
    
    if (keycode == 0) return;
    
    if (screen_state.count < 4) {
        screen_state.count++;
    }
    for (int i = screen_state.count - 1; i > 0; i--) {
        screen_state.history[i] = screen_state.history[i - 1];
    }
    
    screen_state.history[0].timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    screen_state.history[0].device_id = evt->device_id;
    screen_state.history[0].keycode = keycode;
    screen_state.history[0].modifiers = modifiers;
    snprintf(screen_state.history[0].device_name, sizeof(screen_state.history[0].device_name),
             "0x%04X", evt->device_id);
    
    screen_state.lightbar_state = 1 - screen_state.lightbar_state;
    screen_state.last_timestamp = screen_state.history[0].timestamp_ms;
}

void screen_pc_mode_create(lv_obj_t *parent, const statusbar_data_t *initial_status) {
    ESP_LOGI(TAG, "Creating PC mode screen");
    
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    statusbar = ui_compact_statusbar_create(parent);
    
    statusbar_data_t status;
    if (initial_status) {
        status = *initial_status;
    } else {
        status = (statusbar_data_t){
            .usb_connected = false,
            .bluetooth_enabled = false,
            .bluetooth_connected = false,
            .signal_strength = SIGNAL_NONE,
            .battery_level = 0,
            .device_name = "LC-????"
        };
    }
    ui_compact_statusbar_update(statusbar, &status);
    
    // PC MODE text
    mode_label = lv_label_create(parent);
    lv_label_set_text(mode_label, "PC MODE");
    lv_obj_set_style_text_color(mode_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(mode_label, &lv_font_pixolletta_20, 0);
    lv_obj_align(mode_label, LV_ALIGN_CENTER, 0, PRESENTER_TEXT_Y - (DISPLAY_HEIGHT / 2));
    
    // Waiting text (single line, centered)
    waiting_label1 = lv_label_create(parent);
    lv_label_set_text(waiting_label1, "Waiting for commands...");
    lv_obj_set_style_text_color(waiting_label1, lv_color_white(), 0);
    lv_obj_set_style_text_font(waiting_label1, &lv_font_pixolletta_10, 0);
    lv_obj_align(waiting_label1, LV_ALIGN_CENTER, 0, 0);
    
    waiting_label2 = NULL;
    
    // Bottom separator
    lv_obj_t *bottom_line = lv_line_create(parent);
    static lv_point_precise_t bottom_points[] = {{0, SEPARATOR_Y_BOTTOM}, {DISPLAY_WIDTH, SEPARATOR_Y_BOTTOM}};
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);
    
    // Bottom bar
    ui_draw_icon_text(parent, &button_long_press, "Menu", DISPLAY_WIDTH - TEXT_MARGIN_RIGHT, BOTTOM_BAR_Y + 2, UI_ALIGN_RIGHT);
    
    // Subscribe to HID events
    esp_event_loop_handle_t loop = system_events_get_loop();
    esp_event_handler_register_with(loop, SYSTEM_EVENTS, SYSTEM_EVENT_HID_COMMAND_RECEIVED,
                                    hid_command_event_handler, NULL);
    
    ESP_LOGI(TAG, "PC mode screen created");
}
