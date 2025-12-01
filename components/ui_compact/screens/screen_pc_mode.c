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
#include <string.h>

static const char *TAG = "screen_pc_mode";
static lv_obj_t *statusbar = NULL;
static lv_obj_t *mode_label = NULL;
static lv_obj_t *waiting_label1 = NULL;
static lv_obj_t *history_labels[4] = {NULL};
static lv_obj_t *lightbar_rects[4] = {NULL};

static struct {
    command_history_entry_t history[4];
    uint8_t count;
    uint8_t lightbar_state;
    uint32_t last_timestamp;
} screen_state = {0};

static lv_timer_t *update_timer = NULL;

// Keycode to display name lookup
static const char *keycode_to_name(uint8_t keycode, uint8_t modifiers)
{
    static char buf[16];
    const char *key_name = NULL;

    // Letters (a-z)
    if (keycode >= 0x04 && keycode <= 0x1D) {
        buf[0] = 'a' + (keycode - 0x04);
        buf[1] = '\0';
        key_name = buf;
    }
    // Numbers (1-9, 0)
    else if (keycode >= 0x1E && keycode <= 0x26) {
        buf[0] = '1' + (keycode - 0x1E);
        buf[1] = '\0';
        key_name = buf;
    } else if (keycode == 0x27)
        key_name = "0";
    // Function keys
    else if (keycode >= 0x3A && keycode <= 0x45) {
        snprintf(buf, sizeof(buf), "F%d", keycode - 0x39);
        key_name = buf;
    }
    // Special keys
    else {
        switch (keycode) {
        case 0x28: key_name = "Enter"; break;
        case 0x29: key_name = "Esc"; break;
        case 0x2A: key_name = "BkSp"; break;
        case 0x2B: key_name = "Tab"; break;
        case 0x2C: key_name = "Space"; break;
        case 0x2D: key_name = "-"; break;
        case 0x2E: key_name = "="; break;
        case 0x2F: key_name = "["; break;
        case 0x30: key_name = "]"; break;
        case 0x31: key_name = "\\"; break;
        case 0x33: key_name = ";"; break;
        case 0x34: key_name = "'"; break;
        case 0x35: key_name = "`"; break;
        case 0x36: key_name = ","; break;
        case 0x37: key_name = "."; break;
        case 0x38: key_name = "/"; break;
        case 0x4A: key_name = "Home"; break;
        case 0x4B: key_name = "PgUp"; break;
        case 0x4C: key_name = "Del"; break;
        case 0x4D: key_name = "End"; break;
        case 0x4E: key_name = "PgDn"; break;
        case 0x4F: key_name = "→"; break;
        case 0x50: key_name = "←"; break;
        case 0x51: key_name = "↓"; break;
        case 0x52: key_name = "↑"; break;
        default: key_name = "?"; break;
        }
    }

    // Add modifiers
    buf[0] = '\0';
    if (modifiers & 0x01) strcat(buf, "Ctrl+");  // Left Ctrl
    if (modifiers & 0x10) strcat(buf, "Ctrl+");  // Right Ctrl
    if (modifiers & 0x02) strcat(buf, "Shift+"); // Left Shift
    if (modifiers & 0x20) strcat(buf, "Shift+"); // Right Shift
    if (modifiers & 0x04) strcat(buf, "Alt+");   // Left Alt
    if (modifiers & 0x40) strcat(buf, "Alt+");   // Right Alt
    if (modifiers & 0x08) strcat(buf, "Win+");   // Left GUI
    if (modifiers & 0x80) strcat(buf, "Win+");   // Right GUI

    if (buf[0] != '\0') {
        strcat(buf, key_name);
        return buf;
    }

    return key_name;
}

static void update_history_display(lv_timer_t *timer)
{
    (void)timer;

    if (screen_state.count == 0) {
        // Show waiting message
        if (waiting_label1) lv_obj_clear_flag(waiting_label1, LV_OBJ_FLAG_HIDDEN);
        for (int i = 0; i < 4; i++) {
            if (history_labels[i]) lv_obj_add_flag(history_labels[i], LV_OBJ_FLAG_HIDDEN);
            if (lightbar_rects[i]) lv_obj_add_flag(lightbar_rects[i], LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    // Hide waiting message
    if (waiting_label1) lv_obj_add_flag(waiting_label1, LV_OBJ_FLAG_HIDDEN);

    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    for (int i = 0; i < 4; i++) {
        if (i < screen_state.count) {
            uint32_t elapsed_sec = (now_ms - screen_state.history[i].timestamp_ms) / 1000;

            // Lightbar logic: state 0 = lines 2&4 (indices 1&3), state 1 = lines 1&3 (indices 0&2)
            bool draw_lightbar = (screen_state.lightbar_state == 0) ? (i == 1 || i == 3) : (i == 0 || i == 2);

            // Update lightbar visibility
            if (lightbar_rects[i]) {
                if (draw_lightbar) {
                    lv_obj_clear_flag(lightbar_rects[i], LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_add_flag(lightbar_rects[i], LV_OBJ_FLAG_HIDDEN);
                }
            }

            // Update text
            if (history_labels[i]) {
                const char *key_display = keycode_to_name(screen_state.history[i].keycode, screen_state.history[i].modifiers);
                char line[32];
                snprintf(line, sizeof(line), "%04" PRIu32 " %-8s %s", elapsed_sec, screen_state.history[i].device_name, key_display);
                lv_label_set_text(history_labels[i], line);
                lv_obj_clear_flag(history_labels[i], LV_OBJ_FLAG_HIDDEN);

                // Set text color based on lightbar
                lv_obj_set_style_text_color(history_labels[i], draw_lightbar ? lv_color_black() : lv_color_white(), 0);
            }
        } else {
            // Hide unused entries
            if (history_labels[i]) lv_obj_add_flag(history_labels[i], LV_OBJ_FLAG_HIDDEN);
            if (lightbar_rects[i]) lv_obj_add_flag(lightbar_rects[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

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
    screen_state.history[0].device_id = evt->device_id;
    screen_state.history[0].keycode = keycode;
    screen_state.history[0].modifiers = modifiers;
    snprintf(screen_state.history[0].device_name, sizeof(screen_state.history[0].device_name), "0x%04X", evt->device_id);

    // Toggle lightbar on new command
    screen_state.lightbar_state = 1 - screen_state.lightbar_state;
    screen_state.last_timestamp = screen_state.history[0].timestamp_ms;

    // Trigger immediate update
    if (update_timer) {
        lv_timer_ready(update_timer);
    }
}

void screen_pc_mode_create(lv_obj_t *parent, const statusbar_data_t *initial_status)
{
    ESP_LOGI(TAG, "Creating PC mode screen");

    statusbar = ui_compact_statusbar_create(parent);

    statusbar_data_t status;
    if (initial_status) {
        status = *initial_status;
    } else {
        status = (statusbar_data_t){.usb_connected = false,
                                    .bluetooth_enabled = false,
                                    .bluetooth_connected = false,
                                    .signal_strength = SIGNAL_NONE,
                                    .battery_level = 0,
                                    .device_name = "LC-????"};
    }
    ui_compact_statusbar_update(statusbar, &status);

    // Create main screen layout
    const char *device_name = (initial_status && initial_status->device_name) ? initial_status->device_name : "LC-????";
    mode_label = ui_create_main_screen_layout(parent, "PC MODE", device_name);

    // Calculate dynamic layout
    int content_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    int line_height = 9;
    int start_y = SEPARATOR_Y_TOP + 1;

    // Create history display elements (4 lines)
    for (int i = 0; i < 4; i++) {
        int y = start_y + (i * line_height);

        // Lightbar background (white rectangle)
        lightbar_rects[i] = lv_obj_create(parent);
        lv_obj_set_size(lightbar_rects[i], DISPLAY_WIDTH, line_height);
        lv_obj_set_pos(lightbar_rects[i], 0, y);
        lv_obj_set_style_bg_color(lightbar_rects[i], lv_color_white(), 0);
        lv_obj_set_style_bg_opa(lightbar_rects[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(lightbar_rects[i], 0, 0);
        lv_obj_set_style_pad_all(lightbar_rects[i], 0, 0);
        lv_obj_add_flag(lightbar_rects[i], LV_OBJ_FLAG_HIDDEN);

        // Text label
        history_labels[i] = lv_label_create(parent);
        lv_obj_set_style_text_font(history_labels[i], &lv_font_unscii_8, 0);
        lv_obj_set_style_text_color(history_labels[i], lv_color_white(), 0);
        lv_obj_set_pos(history_labels[i], 2, y + 1);
        lv_obj_add_flag(history_labels[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Waiting message - centered
    int content_center_y = SEPARATOR_Y_TOP + (content_height / 2);
    waiting_label1 = lv_label_create(parent);
    lv_label_set_text(waiting_label1, "Waiting for\ncommands...");
    lv_obj_set_style_text_color(waiting_label1, lv_color_white(), 0);
    lv_obj_set_style_text_font(waiting_label1, &lv_font_pixolletta_10, 0);
    lv_obj_set_style_text_align(waiting_label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(waiting_label1, DISPLAY_WIDTH);
    lv_obj_align(waiting_label1, LV_ALIGN_CENTER, 0, content_center_y - SEPARATOR_Y_TOP - 32);

    // Subscribe to HID events
    esp_event_loop_handle_t loop = system_events_get_loop();
    esp_event_handler_register_with(loop, SYSTEM_EVENTS, SYSTEM_EVENT_HID_COMMAND_RECEIVED, hid_command_event_handler, NULL);

    // Start update timer (1 second interval for elapsed time)
    update_timer = lv_timer_create(update_history_display, 1000, NULL);

    ESP_LOGI(TAG, "PC mode screen created");
}

static void screen_pc_mode_destroy(void)
{
    if (update_timer) {
        lv_timer_del(update_timer);
        update_timer = NULL;
    }

    esp_event_loop_handle_t loop = system_events_get_loop();
    esp_event_handler_unregister_with(loop, SYSTEM_EVENTS, SYSTEM_EVENT_HID_COMMAND_RECEIVED, hid_command_event_handler);

    statusbar = NULL;
    mode_label = NULL;
    waiting_label1 = NULL;
    for (int i = 0; i < 4; i++) {
        history_labels[i] = NULL;
        lightbar_rects[i] = NULL;
    }
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
    if (event == INPUT_EVENT_ENCODER_BUTTON_LONG) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#endif
}

static ui_screen_t pc_mode_screen = {
    .type = UI_SCREEN_PC_MODE,
    .create = screen_pc_mode_create_wrapper,
    .destroy = screen_pc_mode_destroy,
    .handle_input_event = handle_input_event,
};

ui_screen_t *screen_pc_mode_get_interface(void)
{
    return &pc_mode_screen;
}
