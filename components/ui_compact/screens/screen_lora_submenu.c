#include "esp_log.h"
#include "ui_strings.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_components.h"
#include "ui_navigator.h"

static const char *TAG = "lora_submenu";
static ui_menu_t *menu = NULL;

const char *LORA_MENU_ITEMS[LORA_MENU_COUNT] = {
    [LORA_MENU_PRESETS] = "Presets", [LORA_MENU_FREQUENCY] = "Frequency", [LORA_MENU_SF] = "Spreading Factor",
    [LORA_MENU_BW] = "Bandwidth",    [LORA_MENU_CR] = "Coding Rate",      [LORA_MENU_TXPOWER] = "TX-Power",
    [LORA_MENU_BAND] = "Band"};

void screen_lora_submenu_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    // Title
    lv_obj_t *title = lv_label_create(parent);
    lv_obj_add_style(title, &style_title, 0);
    lv_label_set_text(title, "LORA SETTINGS");
    lv_obj_set_pos(title, UI_MARGIN_LEFT, 0);

    // Preserve selected index if menu already exists
    int selected = menu ? menu->selected_index : 0;

    // Create menu
    menu                 = ui_menu_create(parent, LORA_MENU_ITEMS, LORA_MENU_COUNT);
    menu->selected_index = selected;
    ui_menu_update(menu, LORA_MENU_ITEMS);
}

void screen_lora_submenu_navigate_down(void)
{
    if (!menu)
        return;
    menu->selected_index = (menu->selected_index + 1) % LORA_MENU_COUNT;
    ui_menu_update(menu, LORA_MENU_ITEMS);
}

void screen_lora_submenu_select(void)
{
    if (!menu)
        return;
    ESP_LOGI(TAG, "LoRa submenu item selected: %d - %s", menu->selected_index, LORA_MENU_ITEMS[menu->selected_index]);
}

int screen_lora_submenu_get_selected(void)
{
    return menu ? menu->selected_index : 0;
}

void screen_lora_submenu_reset(void)
{
    if (menu) {
        free(menu);
        menu = NULL;
    }
}

static void handle_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_SHORT) {
        screen_lora_submenu_navigate_down();
    } else if (event == INPUT_EVENT_NEXT_LONG) {
        screen_lora_submenu_select();
        int selected = screen_lora_submenu_get_selected();
        switch (selected) {
            case LORA_MENU_PRESETS:
                ui_navigator_switch_to(UI_SCREEN_LORA_PRESETS);
                break;
            case LORA_MENU_FREQUENCY:
                ui_navigator_switch_to(UI_SCREEN_LORA_FREQUENCY);
                break;
            case LORA_MENU_SF:
                ui_navigator_switch_to(UI_SCREEN_LORA_SF);
                break;
            case LORA_MENU_BW:
                ui_navigator_switch_to(UI_SCREEN_LORA_BW);
                break;
            case LORA_MENU_CR:
                ui_navigator_switch_to(UI_SCREEN_LORA_CR);
                break;
            case LORA_MENU_TXPOWER:
                ui_navigator_switch_to(UI_SCREEN_LORA_TXPOWER);
                break;
            case LORA_MENU_BAND:
                ui_navigator_switch_to(UI_SCREEN_LORA_BAND);
                break;
            default:
                break;
        }
    } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
        case INPUT_EVENT_ENCODER_BUTTON_SHORT:
            ui_navigator_switch_to(UI_SCREEN_MENU);
            break;
        case INPUT_EVENT_ENCODER_CW:
        case INPUT_EVENT_NEXT_SHORT:
            screen_lora_submenu_navigate_down();
            break;
        case INPUT_EVENT_ENCODER_BUTTON_LONG: {
            screen_lora_submenu_select();
            int selected = screen_lora_submenu_get_selected();
            switch (selected) {
                case LORA_MENU_PRESETS:
                    ui_navigator_switch_to(UI_SCREEN_LORA_PRESETS);
                    break;
                case LORA_MENU_FREQUENCY:
                    ui_navigator_switch_to(UI_SCREEN_LORA_FREQUENCY);
                    break;
                case LORA_MENU_SF:
                    ui_navigator_switch_to(UI_SCREEN_LORA_SF);
                    break;
                case LORA_MENU_BW:
                    ui_navigator_switch_to(UI_SCREEN_LORA_BW);
                    break;
                case LORA_MENU_CR:
                    ui_navigator_switch_to(UI_SCREEN_LORA_CR);
                    break;
                case LORA_MENU_TXPOWER:
                    ui_navigator_switch_to(UI_SCREEN_LORA_TXPOWER);
                    break;
                case LORA_MENU_BAND:
                    ui_navigator_switch_to(UI_SCREEN_LORA_BAND);
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
#endif
}

static ui_screen_t lora_submenu_screen = {
    .type               = UI_SCREEN_LORA_SUBMENU,
    .create             = screen_lora_submenu_create,
    .destroy            = screen_lora_submenu_reset,
    .handle_input_event = handle_input_event,
};

ui_screen_t *screen_lora_submenu_get_interface(void)
{
    return &lora_submenu_screen;
}
