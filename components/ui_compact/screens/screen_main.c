#include "esp_log.h"
#include "general_config.h"
#include "lvgl.h"
#include "presenter_mode_manager.h"
#include "screens.h"
#include "ui_compact_fonts.h"
#include "ui_compact_statusbar.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "ui_navigator.h"

static const char *TAG      = "screen_main";
static lv_obj_t *statusbar  = NULL;
static lv_obj_t *mode_label = NULL;
static lv_group_t *group    = NULL;

// External declarations for generated button icons
LV_IMG_DECLARE(button_short_press);
LV_IMG_DECLARE(button_double_press);
LV_IMG_DECLARE(button_long_press);

void screen_main_create(lv_obj_t *parent, const statusbar_data_t *initial_status)
{
    ESP_LOGI(TAG, "Creating presenter main screen");

    // Create status bar at top
    statusbar = ui_compact_statusbar_create(parent);

    // Use provided status or defaults
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
    mode_label              = ui_create_main_screen_layout(parent, "PRESENTER", device_name);
    ESP_LOGI(TAG, "Device name: %s", device_name);

    // Button hints: Double-press icon + "PREV" on left
    lv_obj_t *double_press_img = lv_img_create(parent);
    lv_img_set_src(double_press_img, &button_double_press);
    lv_obj_set_pos(double_press_img, BUTTON_MARGIN, BUTTON_HINTS_Y);

    lv_obj_t *prev_label = lv_label_create(parent);
    lv_label_set_text(prev_label, "PREV");
    lv_obj_set_style_text_color(prev_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(prev_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(prev_label, BUTTON_MARGIN + button_double_press.header.w + 2, BUTTON_TEXT_Y);

    // Button hints: "NEXT" + short-press icon on right
    lv_obj_t *next_label = lv_label_create(parent);
    lv_label_set_text(next_label, "NEXT");
    lv_obj_set_style_text_color(next_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(next_label, &lv_font_pixolletta_10, 0);
    lv_obj_set_pos(next_label, DISPLAY_WIDTH - BUTTON_MARGIN - button_short_press.header.w - 24 - 2, BUTTON_TEXT_Y);

    lv_obj_t *short_press_img = lv_img_create(parent);
    lv_img_set_src(short_press_img, &button_short_press);
    lv_obj_set_pos(short_press_img, DISPLAY_WIDTH - BUTTON_MARGIN - button_short_press.header.w, BUTTON_HINTS_Y);

    // Create button group for input
    group = lv_group_create();
    lv_group_add_obj(group, mode_label);

    ESP_LOGI(TAG, "Presenter main screen created");
}

void screen_main_update_mode(const char *mode)
{
    if (mode_label) {
        lv_label_set_text(mode_label, mode);
    }
}

void screen_main_update_status(const char *status)
{
    // Reserved for future status updates
}

lv_group_t *screen_main_get_group(void)
{
    return group;
}

static void screen_main_create_wrapper(lv_obj_t *parent)
{
    statusbar_data_t status;
    ui_compact_get_status(&status);

    general_config_t config;
    general_config_get(&config);

    if (config.device_mode == DEVICE_MODE_PC) {
        screen_pc_mode_create(parent, &status);
    } else {
        screen_main_create(parent, &status);
    }
}

static void screen_main_destroy(void)
{
    // Clean up any specific resources if needed
    // LVGL objects are deleted by parent deletion
    group      = NULL;
    statusbar  = NULL;
    mode_label = NULL;
}

static void handle_input(button_event_type_t event)
{
    // Logic from ui_compact.c button_event_handler for UI_SCREEN_MAIN

    // We need to check device mode.
    // Since we don't have direct access to config here, we might need a helper
    // or assume we are in the mode the screen was created for.
    // However, the input handling logic depends on the mode (Presenter vs PC).

    // For now, let's assume we can get the config or delegate.
    // But wait, ui_compact.c logic was:
    // if (config.device_mode == DEVICE_MODE_PRESENTER) { ... }

    // We should probably move the input handling logic here.
    // We need access to general_config_get.

    // Let's add includes for config if needed, or just implement the navigation logic.
    // The navigation logic for Main is:
    // Long press -> Menu
    // Short/Double -> Presenter mode manager (if in presenter mode)

    // We need to include "general_config.h" and "presenter_mode_manager.h"

    general_config_t config;
    general_config_get(&config);

    if (config.device_mode == DEVICE_MODE_PRESENTER) {
        if (event == BUTTON_EVENT_SHORT || event == BUTTON_EVENT_DOUBLE) {
            // Forward to presenter mode manager for HID commands
            // We need to unlock LVGL if we are going to call external things that might block or use other locks?
            // ui_compact.c did ui_lvgl_unlock() before calling presenter_mode_manager_handle_button(event).
            // But ui_navigator_handle_input is called with lock held?
            // No, ui_navigator_handle_input is called from ui_compact.c which holds the lock?
            // Wait, button_event_handler in ui_compact.c holds the lock.
            // So we are inside the lock.

            // If presenter_mode_manager_handle_button needs to be called without lock, we have a problem
            // if we are strictly inside the navigator's handle_input which is called under lock.
            // However, looking at ui_compact.c, it unlocks before calling presenter_mode_manager.

            // We might need to handle this carefully.
            // For now, let's assume we can call it.
            presenter_mode_manager_handle_button(event);
        } else if (event == BUTTON_EVENT_LONG) {
            // Long press -> go to menu
            ui_navigator_switch_to(UI_SCREEN_MENU);
        }
    } else {
        // PC mode: long press -> go to menu
        if (event == BUTTON_EVENT_LONG) {
            ui_navigator_switch_to(UI_SCREEN_MENU);
        }
    }
}

static ui_screen_t main_screen = {
    .type         = UI_SCREEN_MAIN,
    .create       = screen_main_create_wrapper,
    .destroy      = screen_main_destroy,
    .handle_input = handle_input,
};

ui_screen_t *screen_main_get_interface(void)
{
    return &main_screen;
}
