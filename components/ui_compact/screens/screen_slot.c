#include "input_manager.h"
#include "ui_strings.h"
#include "esp_log.h"
#include "input_manager.h"
#include "config_manager.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_components.h"
#include "ui_navigator.h"

static const char *TAG          = "slot_screen";
static ui_edit_screen_t *screen = NULL;
static int selected_slot        = 0;
static bool preserved_edit_mode = false;

void screen_slot_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    if (!screen) {
        screen            = ui_edit_screen_create("SLOT");
        screen->edit_mode = preserved_edit_mode;
    }

    ESP_LOGI(TAG, "Creating slot screen: edit_mode=%d, selected_slot=%d", screen->edit_mode, selected_slot);

    char value_text[32];
    snprintf(value_text, sizeof(value_text), "Slot %d", selected_slot + 1);
    ui_edit_screen_render(screen, parent, "SLOT", value_text, selected_slot, 15);
}

void screen_slot_init(void)
{
    general_config_t config;
    config_manager_get_general(&config);
    selected_slot = config.slot_id - 1;
    if (screen) {
        screen->edit_mode = false;
    }
}

void screen_slot_navigate_down(void)
{
    if (!screen || !screen->edit_mode)
        return;
    selected_slot = (selected_slot + 1) % 16;
}

void screen_slot_navigate_up(void)
{
    if (!screen || !screen->edit_mode)
        return;
    selected_slot = (selected_slot - 1 + 16) % 16;
}

void screen_slot_select(void)
{
    if (!screen)
        return;

    if (screen->edit_mode) {
        general_config_t config;
        config_manager_get_general(&config);
        config.slot_id = selected_slot + 1;
        config_manager_set_general(&config);
        ESP_LOGI(TAG, "Slot saved: %d", config.slot_id);
        screen->edit_mode = false;
        ESP_LOGI(TAG, "Exiting edit mode");
    } else {
        screen->edit_mode = true;
        ESP_LOGI(TAG, "Entering edit mode");
    }
}

bool screen_slot_is_edit_mode(void)
{
    return screen ? screen->edit_mode : false;
}

static void handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (screen_slot_is_edit_mode()) {
        if (event == INPUT_EVENT_NEXT_SHORT) {
            screen_slot_navigate_down();
            ui_navigator_switch_to(UI_SCREEN_SLOT);
        } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
            screen_slot_navigate_up();
            ui_navigator_switch_to(UI_SCREEN_SLOT);
        } else if (event == INPUT_EVENT_NEXT_LONG) {
            screen_slot_select();
            if (!screen_slot_is_edit_mode()) {
                ui_navigator_switch_to(UI_SCREEN_MENU);
            } else {
                ui_navigator_switch_to(UI_SCREEN_SLOT);
            }
        }
    } else {
        if (event == INPUT_EVENT_NEXT_LONG) {
            screen_slot_select();
            ui_navigator_switch_to(UI_SCREEN_SLOT);
        } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
            ui_navigator_switch_to(UI_SCREEN_MENU);
        }
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    if (screen_slot_is_edit_mode()) {
        switch (event) {
            case INPUT_EVENT_PREV_SHORT:
            case INPUT_EVENT_ENCODER_BUTTON_SHORT:
                screen->edit_mode = false;
                ui_navigator_switch_to(UI_SCREEN_MENU);
                break;
            case INPUT_EVENT_ENCODER_CW:
                screen_slot_navigate_down();
                ui_navigator_switch_to(UI_SCREEN_SLOT);
                break;
            case INPUT_EVENT_ENCODER_CCW:
                screen_slot_navigate_up();
                ui_navigator_switch_to(UI_SCREEN_SLOT);
                break;
            case INPUT_EVENT_ENCODER_BUTTON_LONG:
                screen_slot_select();
                ui_navigator_switch_to(UI_SCREEN_MENU);
                break;
            default:
                break;
        }
    } else {
        switch (event) {
            case INPUT_EVENT_PREV_SHORT:
            case INPUT_EVENT_ENCODER_BUTTON_SHORT:
                ui_navigator_switch_to(UI_SCREEN_MENU);
                break;
            case INPUT_EVENT_ENCODER_BUTTON_LONG:
                screen_slot_select();
                ui_navigator_switch_to(UI_SCREEN_SLOT);
                break;
            default:
                break;
        }
    }
#endif
}

static void screen_slot_reset(void)
{
    if (screen) {
        preserved_edit_mode = screen->edit_mode;
        free(screen);
        screen = NULL;
    }
}

static ui_screen_t slot_screen = {
    .type               = UI_SCREEN_SLOT,
    .create             = screen_slot_create,
    .destroy            = screen_slot_reset,
    .handle_input_event = handle_input_event,
};

ui_screen_t *screen_slot_get_interface(void)
{
    return &slot_screen;
}
