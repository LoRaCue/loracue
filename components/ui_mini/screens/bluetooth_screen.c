#include "bluetooth_screen.h"
#include "ble.h"
#include "general_config.h"
#include "ui_mini.h"
#include "u8g2.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_icons.h"

extern u8g2_t u8g2;

typedef enum {
    BT_MENU_MAIN,
    BT_MENU_BLUETOOTH_TOGGLE,
    BT_MENU_PAIRING_TOGGLE,
    BT_MENU_PAIRING_ACTIVE
} bt_menu_state_t;

static bt_menu_state_t menu_state = BT_MENU_MAIN;
static int selected_item = 0;

// Main menu items
static const char *main_items[] = {"Bluetooth", "Pairing"};
static const int main_item_count = 2;

// Toggle menu items
static const char *toggle_items[] = {"ON", "OFF"};
static const int toggle_item_count = 2;

static void draw_main_menu(void)
{
    u8g2_ClearBuffer(&u8g2);
    ui_draw_header("BLUETOOTH");

    general_config_t config;
    general_config_get(&config);

    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height = viewport_height / main_item_count;
    const int bar_height = (viewport_height / 2) - 1;

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    for (int i = 0; i < main_item_count; i++) {
        int item_y_start = SEPARATOR_Y_TOP + 1 + (i * item_height);
        int bar_y_center = item_y_start + (item_height / 2);
        int bar_y = bar_y_center - (bar_height / 2);
        int adjusted_bar_height = bar_height;

        if (i == 0) bar_y += 1;
        if (i == 1) adjusted_bar_height -= 1;

        if (i == selected_item) {
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, adjusted_bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }

        int lightbar_center = bar_y + (adjusted_bar_height / 2);
        int text_y = lightbar_center + 3;

        // Show status
        bool is_on = (i == 0) ? config.bluetooth_enabled : config.bluetooth_pairing_enabled;
        char status_text[48];
        snprintf(status_text, sizeof(status_text), "%s: %s", main_items[i], is_on ? "ON" : "OFF");
        u8g2_DrawStr(&u8g2, 4, text_y, status_text);

        if (i == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }

    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);
    u8g2_SendBuffer(&u8g2);
}

static void draw_toggle_menu(const char *title, bool current_state)
{
    u8g2_ClearBuffer(&u8g2);
    ui_draw_header(title);

    const int viewport_height = SEPARATOR_Y_BOTTOM - SEPARATOR_Y_TOP;
    const int item_height = viewport_height / toggle_item_count;
    const int bar_height = (viewport_height / 2) - 1;

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);

    for (int i = 0; i < toggle_item_count; i++) {
        int item_y_start = SEPARATOR_Y_TOP + 1 + (i * item_height);
        int bar_y_center = item_y_start + (item_height / 2);
        int bar_y = bar_y_center - (bar_height / 2);
        int adjusted_bar_height = bar_height;

        if (i == 0) bar_y += 1;
        if (i == 1) adjusted_bar_height -= 1;

        if (i == selected_item) {
            u8g2_DrawBox(&u8g2, 0, bar_y, DISPLAY_WIDTH, adjusted_bar_height);
            u8g2_SetDrawColor(&u8g2, 0);
        }

        int lightbar_center = bar_y + (adjusted_bar_height / 2);
        int text_y = lightbar_center + 3;

        // Show checkmark for active state
        if ((i == 0 && current_state) || (i == 1 && !current_state)) {
            int icon_y = lightbar_center - (checkmark_height / 2);
            u8g2_DrawXBM(&u8g2, 4, icon_y, checkmark_width, checkmark_height, checkmark_bits);
            u8g2_DrawStr(&u8g2, 16, text_y, toggle_items[i]);
        } else {
            u8g2_DrawStr(&u8g2, 16, text_y, toggle_items[i]);
        }

        if (i == selected_item) {
            u8g2_SetDrawColor(&u8g2, 1);
        }
    }

    ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);
    u8g2_SendBuffer(&u8g2);
}

static void draw_pairing_screen(void)
{
    u8g2_ClearBuffer(&u8g2);
    ui_draw_header("PAIRING");

    uint32_t passkey = 0;
    bool pairing_active = ble_get_passkey(&passkey);

    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    if (pairing_active) {
        // Show passkey
        u8g2_DrawStr(&u8g2, 4, 24, "Enter passkey:");
        
        char passkey_str[8];
        snprintf(passkey_str, sizeof(passkey_str), "%06lu", (unsigned long)passkey);
        
        u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);
        int text_width = u8g2_GetStrWidth(&u8g2, passkey_str);
        int text_x = (DISPLAY_WIDTH - text_width) / 2;
        u8g2_DrawStr(&u8g2, text_x, 42, passkey_str);
        
        u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawStr(&u8g2, 4, 56, "Press button to abort");
    } else {
        u8g2_DrawStr(&u8g2, 4, 32, "Waiting for pairing");
        u8g2_DrawStr(&u8g2, 4, 44, "request...");
    }

    ui_draw_footer(FOOTER_CONTEXT_INFO, NULL);
    u8g2_SendBuffer(&u8g2);
}

void bluetooth_screen_draw(void)
{
    general_config_t config;
    general_config_get(&config);

    switch (menu_state) {
        case BT_MENU_MAIN:
            draw_main_menu();
            break;
        case BT_MENU_BLUETOOTH_TOGGLE:
            draw_toggle_menu("BLUETOOTH", config.bluetooth_enabled);
            break;
        case BT_MENU_PAIRING_TOGGLE:
            draw_toggle_menu("PAIRING", config.bluetooth_pairing_enabled);
            break;
        case BT_MENU_PAIRING_ACTIVE:
            draw_pairing_screen();
            break;
    }
}

void bluetooth_screen_handle_input(int button)
{
    general_config_t config;
    general_config_get(&config);

    if (menu_state == BT_MENU_MAIN) {
        if (button == 0) { // UP
            selected_item = (selected_item - 1 + main_item_count) % main_item_count;
            bluetooth_screen_draw();
        } else if (button == 1) { // DOWN
            selected_item = (selected_item + 1) % main_item_count;
            bluetooth_screen_draw();
        } else if (button == 2) { // SELECT
            if (selected_item == 0) {
                // Enter Bluetooth toggle menu
                menu_state = BT_MENU_BLUETOOTH_TOGGLE;
                selected_item = config.bluetooth_enabled ? 0 : 1;
            } else {
                // Enter Pairing toggle menu
                menu_state = BT_MENU_PAIRING_TOGGLE;
                selected_item = config.bluetooth_pairing_enabled ? 0 : 1;
            }
            bluetooth_screen_draw();
        }
    } else if (menu_state == BT_MENU_BLUETOOTH_TOGGLE) {
        if (button == 0) { // UP
            selected_item = (selected_item - 1 + toggle_item_count) % toggle_item_count;
            bluetooth_screen_draw();
        } else if (button == 1) { // DOWN
            selected_item = (selected_item + 1) % toggle_item_count;
            bluetooth_screen_draw();
        } else if (button == 2) { // SELECT
            config.bluetooth_enabled = (selected_item == 0);
            general_config_set(&config);
            ble_set_enabled(config.bluetooth_enabled);
            
            // Return to main menu
            menu_state = BT_MENU_MAIN;
            selected_item = 0;
            bluetooth_screen_draw();
        }
    } else if (menu_state == BT_MENU_PAIRING_TOGGLE) {
        if (button == 0) { // UP
            selected_item = (selected_item - 1 + toggle_item_count) % toggle_item_count;
            bluetooth_screen_draw();
        } else if (button == 1) { // DOWN
            selected_item = (selected_item + 1) % toggle_item_count;
            bluetooth_screen_draw();
        } else if (button == 2) { // SELECT
            config.bluetooth_pairing_enabled = (selected_item == 0);
            general_config_set(&config);
            
            // If enabling pairing, show pairing screen
            if (config.bluetooth_pairing_enabled) {
                menu_state = BT_MENU_PAIRING_ACTIVE;
            } else {
                menu_state = BT_MENU_MAIN;
                selected_item = 1;
            }
            bluetooth_screen_draw();
        }
    } else if (menu_state == BT_MENU_PAIRING_ACTIVE) {
        // Any button press aborts pairing
        config.bluetooth_pairing_enabled = false;
        general_config_set(&config);
        
        menu_state = BT_MENU_MAIN;
        selected_item = 1;
        bluetooth_screen_draw();
    }
}
