#include "ble.h"
#include "esp_log.h"
#include "general_config.h"
#include "lvgl.h"
#include "screens.h"
#include "ui_components.h"
#include "ui_lvgl_config.h"
#include "ui_navigator.h"

static const char *TAG = "bluetooth";
static ui_menu_t *menu = NULL;

void screen_bluetooth_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    ui_create_header(parent, "BLUETOOTH");
    ui_create_footer(parent);

    general_config_t config;
    general_config_get(&config);

    static char bt_text[32], pair_text[32];
    snprintf(bt_text, sizeof(bt_text), "Bluetooth: %s", config.bluetooth_enabled ? "ON" : "OFF");
    snprintf(pair_text, sizeof(pair_text), "Pairing: %s", config.bluetooth_pairing_enabled ? "ON" : "OFF");

    const char *items[] = {bt_text, pair_text};
    menu                = ui_menu_create(parent, items, 2);
    ui_menu_update(menu, items);
}

void screen_bluetooth_navigate_down(void)
{
    if (!menu)
        return;
    menu->selected_index = (menu->selected_index + 1) % 2;

    general_config_t config;
    general_config_get(&config);

    static char bt_text[32], pair_text[32];
    snprintf(bt_text, sizeof(bt_text), "Bluetooth: %s", config.bluetooth_enabled ? "ON" : "OFF");
    snprintf(pair_text, sizeof(pair_text), "Pairing: %s", config.bluetooth_pairing_enabled ? "ON" : "OFF");

    const char *items[] = {bt_text, pair_text};
    ui_menu_update(menu, items);
}

void screen_bluetooth_select(void)
{
    if (!menu)
        return;

    general_config_t config;
    general_config_get(&config);

    if (menu->selected_index == 0) {
        config.bluetooth_enabled = !config.bluetooth_enabled;
        ble_set_enabled(config.bluetooth_enabled);
    } else {
        config.bluetooth_pairing_enabled = !config.bluetooth_pairing_enabled;
    }

    general_config_set(&config);
    ESP_LOGI(TAG, "Bluetooth setting toggled: item %d", menu->selected_index);
}

void screen_bluetooth_reset(void)
{
    if (menu) {
        free(menu);
        menu = NULL;
    }
}

static void handle_input(button_event_type_t event)
{
    if (event == BUTTON_EVENT_SHORT) {
        screen_bluetooth_navigate_down();
    } else if (event == BUTTON_EVENT_LONG) {
        screen_bluetooth_select();
        // Re-create screen to update UI
        // In the new system, we might need to trigger a redraw or re-create
        // Since we are inside the screen's input handler, we can't easily "reload"
        // without exiting and entering.
        // But wait, the legacy logic did:
        // lv_obj_t *screen = lv_obj_create(NULL);
        // screen_bluetooth_create(screen);
        // lv_screen_load(screen);

        // We can achieve this by calling create again on the current parent?
        // No, we don't have reference to parent.

        // Better approach: Update the menu items directly.
        // screen_bluetooth_select updates the config.
        // We should update the menu text.

        // Actually, screen_bluetooth_navigate_down also updates the text!
        // Let's check screen_bluetooth_select implementation.
        // It toggles config but doesn't update menu text.

        // We should call a refresh function.
        // screen_bluetooth_navigate_down updates the menu items.
        // Let's reuse that logic or create a refresh function.

        // For now, let's just call navigate_down which updates the text,
        // but that also changes selection.

        // Let's modify screen_bluetooth_select to update the menu.
        // Or just call create again? No.

        // Let's look at screen_bluetooth_navigate_down. It does:
        // menu->selected_index = ...
        // ui_menu_update(menu, items);

        // We can just call ui_menu_update with new items.
        // But we need to construct the items string again.

        // Let's just trigger a reload via navigator?
        // ui_navigator_switch_to(UI_SCREEN_BLUETOOTH);
        // This works because switch_to destroys and creates.
        ui_navigator_switch_to(UI_SCREEN_BLUETOOTH);

    } else if (event == BUTTON_EVENT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
}

static ui_screen_t bluetooth_screen = {
    .type         = UI_SCREEN_BLUETOOTH,
    .create       = screen_bluetooth_create,
    .destroy      = screen_bluetooth_reset,
    .handle_input = handle_input,
};

ui_screen_t *screen_bluetooth_get_interface(void)
{
    return &bluetooth_screen;
}
