#include "lvgl.h"
#include "input_manager.h"
#include "screens.h"
#include "ui_components.h"
#include "ui_navigator.h"

static ui_menu_t *menu = NULL;

const char *MAIN_MENU_ITEMS[MAIN_MENU_COUNT] = {
    [MAIN_MENU_DEVICE_MODE] = "Device Mode",    [MAIN_MENU_SLOT] = "Slot",
    [MAIN_MENU_LORA] = "LoRa Settings",         [MAIN_MENU_PAIRING] = "Device Pairing",
    [MAIN_MENU_REGISTRY] = "Device Registry",   [MAIN_MENU_CONTRAST] = "Display Contrast",
    [MAIN_MENU_BLUETOOTH] = "Bluetooth",        [MAIN_MENU_CONFIG] = "Configuration Mode",
    [MAIN_MENU_DEVICE_INFO] = "Device Info",    [MAIN_MENU_SYSTEM_INFO] = "System Info",
    [MAIN_MENU_FACTORY_RESET] = "Factory Reset"};

void screen_menu_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    // Preserve selected index if menu already exists
    int selected = menu ? menu->selected_index : 0;

    menu                 = ui_menu_create(parent, MAIN_MENU_ITEMS, MAIN_MENU_COUNT);
    menu->selected_index = selected;
    ui_menu_update(menu, MAIN_MENU_ITEMS);
}

void screen_menu_navigate_down(void)
{
    if (!menu)
        return;
    int new_index = (menu->selected_index + 1) % MAIN_MENU_COUNT;
    if (new_index != menu->selected_index) {
        menu->selected_index = new_index;
        ui_menu_update(menu, MAIN_MENU_ITEMS);
    }
}

int screen_menu_get_selected(void)
{
    return menu ? menu->selected_index : 0;
}

void screen_menu_reset(void)
{
    if (menu) {
        menu->selected_index = 0;
        free(menu);
        menu = NULL;
    }
}

const char *screen_menu_get_selected_name(void)
{
    if (menu && menu->selected_index >= 0 && menu->selected_index < MAIN_MENU_COUNT) {
        return MAIN_MENU_ITEMS[menu->selected_index];
    }
    return NULL;
}

static void handle_input(button_event_type_t event)
{
    if (event == BUTTON_EVENT_SHORT) {
        screen_menu_navigate_down();
    } else if (event == BUTTON_EVENT_LONG) {
        int selected = screen_menu_get_selected();

        switch (selected) {
            case MAIN_MENU_DEVICE_MODE:
                ui_navigator_switch_to(UI_SCREEN_DEVICE_MODE);
                break;
            case MAIN_MENU_SLOT:
                ui_navigator_switch_to(UI_SCREEN_SLOT);
                break;
            case MAIN_MENU_LORA:
                ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
                break;
            case MAIN_MENU_PAIRING:
                ui_navigator_switch_to(UI_SCREEN_PAIRING);
                break;
            case MAIN_MENU_REGISTRY:
                ui_navigator_switch_to(UI_SCREEN_DEVICE_REGISTRY);
                break;
            case MAIN_MENU_CONTRAST:
                ui_navigator_switch_to(UI_SCREEN_CONTRAST);
                break;
            case MAIN_MENU_BLUETOOTH:
                ui_navigator_switch_to(UI_SCREEN_BLUETOOTH);
                break;
            case MAIN_MENU_CONFIG:
                ui_navigator_switch_to(UI_SCREEN_CONFIG_MODE);
                break;
            case MAIN_MENU_DEVICE_INFO:
                ui_navigator_switch_to(UI_SCREEN_DEVICE_INFO);
                break;
            case MAIN_MENU_SYSTEM_INFO:
                ui_navigator_switch_to(UI_SCREEN_SYSTEM_INFO);
                break;
            case MAIN_MENU_FACTORY_RESET:
                ui_navigator_switch_to(UI_SCREEN_FACTORY_RESET);
                break;
            default:
                break;
        }
    } else if (event == BUTTON_EVENT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MAIN);
    }
}

#if CONFIG_INPUT_HAS_DUAL_BUTTONS
static void handle_input_event(input_event_t event)
{
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
            ui_navigator_switch_to(UI_SCREEN_MAIN);
            break;
        case INPUT_EVENT_ENCODER_CW:
            screen_menu_navigate_down();
            break;
        case INPUT_EVENT_ENCODER_CCW:
            // Navigate up
            if (menu) {
                int new_index = (menu->selected_index - 1 + MAIN_MENU_COUNT) % MAIN_MENU_COUNT;
                if (new_index != menu->selected_index) {
                    menu->selected_index = new_index;
                    ui_menu_update(menu, MAIN_MENU_ITEMS);
                }
            }
            break;
        case INPUT_EVENT_NEXT_SHORT:
        case INPUT_EVENT_ENCODER_BUTTON: {
            // Select current item (Alpha+)
            int selected = screen_menu_get_selected();
            switch (selected) {
                case MAIN_MENU_DEVICE_MODE:
                    ui_navigator_switch_to(UI_SCREEN_DEVICE_MODE);
                    break;
                case MAIN_MENU_SLOT:
                    ui_navigator_switch_to(UI_SCREEN_SLOT);
                    break;
                case MAIN_MENU_LORA:
                    ui_navigator_switch_to(UI_SCREEN_LORA_SUBMENU);
                    break;
                case MAIN_MENU_PAIRING:
                    ui_navigator_switch_to(UI_SCREEN_PAIRING);
                    break;
                case MAIN_MENU_REGISTRY:
                    ui_navigator_switch_to(UI_SCREEN_DEVICE_REGISTRY);
                    break;
                case MAIN_MENU_CONTRAST:
                    ui_navigator_switch_to(UI_SCREEN_CONTRAST);
                    break;
                case MAIN_MENU_BLUETOOTH:
                    ui_navigator_switch_to(UI_SCREEN_BLUETOOTH);
                    break;
                case MAIN_MENU_CONFIG:
                    ui_navigator_switch_to(UI_SCREEN_CONFIG_MODE);
                    break;
                case MAIN_MENU_DEVICE_INFO:
                    ui_navigator_switch_to(UI_SCREEN_DEVICE_INFO);
                    break;
                case MAIN_MENU_SYSTEM_INFO:
                    ui_navigator_switch_to(UI_SCREEN_SYSTEM_INFO);
                    break;
                case MAIN_MENU_FACTORY_RESET:
                    ui_navigator_switch_to(UI_SCREEN_FACTORY_RESET);
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}
#endif

static ui_screen_t menu_screen = {
    .type         = UI_SCREEN_MENU,
    .create       = screen_menu_create,
    .destroy      = screen_menu_reset,
    .handle_input = handle_input,
#if CONFIG_INPUT_HAS_DUAL_BUTTONS
    .handle_input_event = handle_input_event,
#endif
};

ui_screen_t *screen_menu_get_interface(void)
{
    return &menu_screen;
}
