#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "screens.h"
#include "system_events.h"
#include "ui_compact.h"
#include "ui_compact_fonts.h"
#include "ui_compact_statusbar.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "ui_navigator.h"
#include <inttypes.h>

static const char *TAG          = "screen_pc_mode";
static lv_obj_t *statusbar      = NULL;
static lv_obj_t *mode_label     = NULL;
static lv_obj_t *waiting_label1 = NULL;
static lv_obj_t *waiting_label2 = NULL;

static struct {
    command_history_entry_t history[4];
    uint8_t count;
    uint8_t lightbar_state;
    uint32_t last_timestamp;
} screen_state = {0};

LV_IMG_DECLARE(button_long_press);

static void hid_command_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const system_event_hid_command_t *evt = (const system_event_hid_command_t *)data;

    uint8_t modifiers = 0, keycode = 0;
    system_event_get_keyboard_data(evt, &modifiers, &keycode);

    if (keycode == 0)
        return;

    if (screen_state.count < 4) {
        screen_state.count++;
    }
    for (int i = screen_state.count - 1; i > 0; i--) {
        screen_state.history[i] = screen_state.history[i - 1];
    }

    screen_state.history[0].timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    screen_state.history[0].device_id    = evt->device_id;
    screen_state.history[0].keycode      = keycode;
    screen_state.history[0].modifiers    = modifiers;
    snprintf(screen_state.history[0].device_name, sizeof(screen_state.history[0].device_name), "0x%04X",
             evt->device_id);

    screen_state.lightbar_state = 1 - screen_state.lightbar_state;
    screen_state.last_timestamp = screen_state.history[0].timestamp_ms;
}

void screen_pc_mode_create(lv_obj_t *parent, const statusbar_data_t *initial_status)
{
    ESP_LOGI(TAG, "Creating PC mode screen");

    statusbar = ui_compact_statusbar_create(parent);

    statusbar_data_t status;
    if (initial_status) {
        status = *initial_status;
    } else {
        status = (statusbar_data_t){.usb_connected       = false,
                                    .bluetooth_enabled   = false,
                                    .bluetooth_connected = false,
                                    .signal_strength     = SIGNAL_NONE,
                                    .battery_level       = 0,
                                    .device_name         = "LC-????"};
    }
    ui_compact_statusbar_update(statusbar, &status);

    // Create main screen layout (mode label, bottom bar with device name and menu)
    const char *device_name = (initial_status && initial_status->device_name) ? initial_status->device_name : "LC-????";
    mode_label              = ui_create_main_screen_layout(parent, "PC MODE", device_name);

    // Waiting text - centered in content area, 10px down
    int content_height   = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    int content_center_y = SEPARATOR_Y_TOP + (content_height / 2) + 10; // +10px down

    waiting_label1 = lv_label_create(parent);
    lv_label_set_text(waiting_label1, "Waiting for commands...");
    lv_obj_set_style_text_color(waiting_label1, lv_color_white(), 0);
    lv_obj_set_style_text_font(waiting_label1, &lv_font_pixolletta_10, 0);
    lv_obj_set_style_text_align(waiting_label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(waiting_label1, DISPLAY_WIDTH);
    lv_obj_set_pos(waiting_label1, 0, content_center_y - 5); // -5 to center 10px font

    waiting_label2 = NULL;

    // Subscribe to HID events
    esp_event_loop_handle_t loop = system_events_get_loop();
    esp_event_handler_register_with(loop, SYSTEM_EVENTS, SYSTEM_EVENT_HID_COMMAND_RECEIVED, hid_command_event_handler,
                                    NULL);

    ESP_LOGI(TAG, "PC mode screen created");
}

static void screen_pc_mode_destroy(void)
{
    esp_event_loop_handle_t loop = system_events_get_loop();
    esp_event_handler_unregister_with(loop, SYSTEM_EVENTS, SYSTEM_EVENT_HID_COMMAND_RECEIVED,
                                      hid_command_event_handler);
    statusbar      = NULL;
    mode_label     = NULL;
    waiting_label1 = NULL;
    waiting_label2 = NULL;
}

static void screen_pc_mode_create_wrapper(lv_obj_t *parent)
{
    statusbar_data_t status;
    ui_compact_get_status(&status);
    screen_pc_mode_create(parent, &status);
}

static void handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_LONG) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    if (event == INPUT_EVENT_ENCODER_BUTTON_SHORT) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#endif
}

static ui_screen_t pc_mode_screen = {
    .type               = UI_SCREEN_PC_MODE,
    .create             = screen_pc_mode_create_wrapper,
    .destroy            = screen_pc_mode_destroy,
    .handle_input_event = handle_input_event,
};

ui_screen_t *screen_pc_mode_get_interface(void)
{
    return &pc_mode_screen;
}
