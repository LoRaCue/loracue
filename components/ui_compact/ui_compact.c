#include "ui_compact.h"
#include "ui_lvgl.h"
#include "screens.h"
#include "ui_components.h"
#include "button_manager.h"
#include "common_types.h"
#include "general_config.h"
#include "power_mgmt.h"
#include "ble.h"
#include "bsp.h"
#include "lora_protocol.h"
#include "system_events.h"
#include "presenter_mode_manager.h"
#include "tusb.h"
#include "esp_log.h"

static const char *TAG = "ui_compact";

static lv_obj_t *current_screen = NULL;
static ui_screen_type_t current_screen_type = UI_SCREEN_BOOT;
static general_config_t cached_config;

static void get_current_status(statusbar_data_t *status) {
    general_config_get(&cached_config);
    
    // Get USB host connection status from TinyUSB
    status->usb_connected = tud_mounted();
    
    // Get Bluetooth status
    status->bluetooth_enabled = ble_is_enabled();
    status->bluetooth_connected = ble_is_connected();
    
    // Get LoRa signal strength
    lora_connection_state_t conn_state = lora_protocol_get_connection_state();
    switch (conn_state) {
        case LORA_CONNECTION_EXCELLENT:
            status->signal_strength = SIGNAL_STRONG;
            break;
        case LORA_CONNECTION_GOOD:
            status->signal_strength = SIGNAL_GOOD;
            break;
        case LORA_CONNECTION_WEAK:
            status->signal_strength = SIGNAL_FAIR;
            break;
        case LORA_CONNECTION_POOR:
            status->signal_strength = SIGNAL_WEAK;
            break;
        case LORA_CONNECTION_LOST:
        default:
            status->signal_strength = SIGNAL_NONE;
            break;
    }
    
    // Get battery level from BSP (convert voltage to percentage)
    float battery_voltage = bsp_read_battery();
    if (battery_voltage >= 4.2f) {
        status->battery_level = 100;
    } else if (battery_voltage <= 3.0f) {
        status->battery_level = 0;
    } else {
        status->battery_level = (uint8_t)((battery_voltage - 3.0f) / 1.2f * 100);
    }
    
    status->device_name = cached_config.device_name;
}

static void handle_menu_selection(int selected_index, const char *item_name) {
    ESP_LOGI(TAG, "Menu item selected: %d - %s", selected_index, item_name);
    
    lv_obj_t *screen = NULL;
    
    switch (selected_index) {
        case MAIN_MENU_DEVICE_MODE:
            screen_device_mode_reset();
            screen = lv_obj_create(NULL);
            screen_device_mode_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_DEVICE_MODE;
            break;
        case MAIN_MENU_SLOT:
            screen_slot_init();
            screen = lv_obj_create(NULL);
            screen_slot_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_SLOT;
            break;
        case MAIN_MENU_LORA:
            screen_lora_submenu_reset();
            screen = lv_obj_create(NULL);
            screen_lora_submenu_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_LORA_SUBMENU;
            break;
        case MAIN_MENU_PAIRING:
            screen_pairing_reset();
            screen = lv_obj_create(NULL);
            screen_pairing_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_PAIRING;
            break;
        case MAIN_MENU_REGISTRY:
            screen_device_registry_reset();
            screen = lv_obj_create(NULL);
            screen_device_registry_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_DEVICE_REGISTRY;
            break;
        case MAIN_MENU_BRIGHTNESS:
            screen_brightness_init();
            screen = lv_obj_create(NULL);
            screen_brightness_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_BRIGHTNESS;
            break;
        case MAIN_MENU_BATTERY:
            screen = lv_obj_create(NULL);
            screen_battery_status_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_BATTERY;
            break;
        case MAIN_MENU_BLUETOOTH:
            screen_bluetooth_reset();
            screen = lv_obj_create(NULL);
            screen_bluetooth_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_BLUETOOTH;
            break;
        case MAIN_MENU_CONFIG:
            screen_config_mode_reset();
            screen = lv_obj_create(NULL);
            screen_config_mode_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_CONFIG_MODE;
            break;
        case MAIN_MENU_DEVICE_INFO:
            screen = lv_obj_create(NULL);
            screen_device_info_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_DEVICE_INFO;
            break;
        case MAIN_MENU_SYSTEM_INFO:
            screen = lv_obj_create(NULL);
            screen_system_info_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_SYSTEM_INFO;
            break;
        case MAIN_MENU_FACTORY_RESET:
            screen = lv_obj_create(NULL);
            screen_factory_reset_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
            current_screen_type = UI_SCREEN_FACTORY_RESET;
            break;
        default:
            ESP_LOGW(TAG, "Unknown menu item: %d", selected_index);
            break;
    }
}

static void button_event_handler(button_event_type_t event, void *arg) {
    ESP_LOGI(TAG, "Button event received: %d on screen %d", event, current_screen_type);
    
    ui_lvgl_lock();
    
    if (current_screen_type == UI_SCREEN_MAIN) {
        general_config_t config;
        general_config_get(&config);
        
        if (config.device_mode == DEVICE_MODE_PRESENTER) {
            if (event == BUTTON_EVENT_SHORT || event == BUTTON_EVENT_DOUBLE) {
                // Forward to presenter mode manager for HID commands
                ui_lvgl_unlock();
                presenter_mode_manager_handle_button(event);
                return;
            } else if (event == BUTTON_EVENT_LONG) {
                // Long press -> go to menu
                ESP_LOGI(TAG, "Switching to menu screen");
                lv_obj_t *menu_screen = lv_obj_create(NULL);
                screen_menu_create(menu_screen);
                lv_screen_load(menu_screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = menu_screen;
                current_screen_type = UI_SCREEN_MENU;
            }
        } else if (event == BUTTON_EVENT_LONG) {
            // PC mode: long press -> go to menu
            ESP_LOGI(TAG, "Switching to menu screen");
            lv_obj_t *menu_screen = lv_obj_create(NULL);
            screen_menu_create(menu_screen);
            lv_screen_load(menu_screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = menu_screen;
            current_screen_type = UI_SCREEN_MENU;
        }
    } else if (current_screen_type == UI_SCREEN_MENU) {
        if (event == BUTTON_EVENT_SHORT) {
            // Short press on menu -> navigate to next item
            ESP_LOGI(TAG, "Menu: navigate down");
            screen_menu_navigate_down();
        } else if (event == BUTTON_EVENT_LONG) {
            // Long press on menu -> select item
            int selected = screen_menu_get_selected();
            const char *item_name = screen_menu_get_selected_name();
            ESP_LOGI(TAG, "Menu: selected item %d - %s", selected, item_name);
            handle_menu_selection(selected, item_name);
        } else if (event == BUTTON_EVENT_DOUBLE) {
            // Double press on menu -> go back to main
            ESP_LOGI(TAG, "Going back to main screen");
            ui_compact_show_main_screen();
        }
    } else if (current_screen_type == UI_SCREEN_BATTERY ||
               current_screen_type == UI_SCREEN_DEVICE_INFO ||
               current_screen_type == UI_SCREEN_SYSTEM_INFO ||
               current_screen_type == UI_SCREEN_PAIRING ||
               current_screen_type == UI_SCREEN_DEVICE_REGISTRY ||
               current_screen_type == UI_SCREEN_CONFIG_MODE) {
        if (event == BUTTON_EVENT_DOUBLE) {
            // Double press -> go back to menu
            ESP_LOGI(TAG, "Going back to menu");
            lv_obj_t *menu_screen = lv_obj_create(NULL);
            screen_menu_create(menu_screen);
            lv_screen_load(menu_screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = menu_screen;
            current_screen_type = UI_SCREEN_MENU;
        }
    } else if (current_screen_type == UI_SCREEN_LORA_SUBMENU) {
        if (event == BUTTON_EVENT_SHORT) {
            screen_lora_submenu_navigate_down();
            lv_obj_t *screen = lv_obj_create(NULL);
            screen_lora_submenu_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
        } else if (event == BUTTON_EVENT_LONG) {
            screen_lora_submenu_select();
            int selected = screen_lora_submenu_get_selected();
            
            lv_obj_t *screen = lv_obj_create(NULL);
            
            switch (selected) {
                case LORA_MENU_PRESETS:
                    screen_lora_presets_reset();
                    screen_lora_presets_create(screen);
                    current_screen_type = UI_SCREEN_LORA_PRESETS;
                    break;
                case LORA_MENU_FREQUENCY:
                    screen_lora_frequency_init();
                    screen_lora_frequency_create(screen);
                    current_screen_type = UI_SCREEN_LORA_FREQUENCY;
                    break;
                case 2: // Spreading Factor
                    screen_lora_sf_init();
                    screen_lora_sf_create(screen);
                    current_screen_type = UI_SCREEN_LORA_SF;
                    break;
                case 3: // Bandwidth
                    screen_lora_bw_init();
                    screen_lora_bw_create(screen);
                    current_screen_type = UI_SCREEN_LORA_BW;
                    break;
                default:
                    // Other items not implemented yet
                    lv_obj_del(screen);
                    return;
            }
            
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
        } else if (event == BUTTON_EVENT_DOUBLE) {
            lv_obj_t *menu_screen = lv_obj_create(NULL);
            screen_menu_create(menu_screen);
            lv_screen_load(menu_screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = menu_screen;
            current_screen_type = UI_SCREEN_MENU;
        }
    } else if (current_screen_type == UI_SCREEN_BLUETOOTH) {
        if (event == BUTTON_EVENT_SHORT) {
            screen_bluetooth_navigate_down();
            lv_obj_t *screen = lv_obj_create(NULL);
            screen_bluetooth_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
        } else if (event == BUTTON_EVENT_LONG) {
            screen_bluetooth_select();
            lv_obj_t *screen = lv_obj_create(NULL);
            screen_bluetooth_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
        } else if (event == BUTTON_EVENT_DOUBLE) {
            lv_obj_t *menu_screen = lv_obj_create(NULL);
            screen_menu_create(menu_screen);
            lv_screen_load(menu_screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = menu_screen;
            current_screen_type = UI_SCREEN_MENU;
        }
    } else if (current_screen_type == UI_SCREEN_SLOT) {
        if (screen_slot_is_edit_mode()) {
            if (event == BUTTON_EVENT_SHORT) {
                screen_slot_navigate_down();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_slot_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_DOUBLE) {
                screen_slot_navigate_up();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_slot_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_LONG) {
                screen_slot_select();
                lv_obj_t *menu_screen = lv_obj_create(NULL);
                screen_menu_create(menu_screen);
                lv_screen_load(menu_screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = menu_screen;
                current_screen_type = UI_SCREEN_MENU;
            }
        } else {
            if (event == BUTTON_EVENT_LONG) {
                screen_slot_select();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_slot_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_DOUBLE) {
                lv_obj_t *menu_screen = lv_obj_create(NULL);
                screen_menu_create(menu_screen);
                lv_screen_load(menu_screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = menu_screen;
                current_screen_type = UI_SCREEN_MENU;
            }
        }
    } else if (current_screen_type == UI_SCREEN_BRIGHTNESS) {
        if (screen_brightness_is_edit_mode()) {
            if (event == BUTTON_EVENT_SHORT) {
                screen_brightness_navigate_down();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_brightness_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_DOUBLE) {
                screen_brightness_navigate_up();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_brightness_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_LONG) {
                screen_brightness_select();
                lv_obj_t *menu_screen = lv_obj_create(NULL);
                screen_menu_create(menu_screen);
                lv_screen_load(menu_screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = menu_screen;
                current_screen_type = UI_SCREEN_MENU;
            }
        } else {
            if (event == BUTTON_EVENT_LONG) {
                screen_brightness_select();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_brightness_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_DOUBLE) {
                lv_obj_t *menu_screen = lv_obj_create(NULL);
                screen_menu_create(menu_screen);
                lv_screen_load(menu_screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = menu_screen;
                current_screen_type = UI_SCREEN_MENU;
            }
        }
    } else if (current_screen_type == UI_SCREEN_DEVICE_MODE) {
        if (event == BUTTON_EVENT_SHORT) {
            // Navigate down
            screen_device_mode_navigate_down();
            lv_obj_t *screen = lv_obj_create(NULL);
            screen_device_mode_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
        } else if (event == BUTTON_EVENT_LONG) {
            // Select and return to menu
            screen_device_mode_select();
            lv_obj_t *menu_screen = lv_obj_create(NULL);
            screen_menu_create(menu_screen);
            lv_screen_load(menu_screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = menu_screen;
            current_screen_type = UI_SCREEN_MENU;
        } else if (event == BUTTON_EVENT_DOUBLE) {
            // Back to menu
            lv_obj_t *menu_screen = lv_obj_create(NULL);
            screen_menu_create(menu_screen);
            lv_screen_load(menu_screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = menu_screen;
            current_screen_type = UI_SCREEN_MENU;
        }
    } else if (current_screen_type == UI_SCREEN_FACTORY_RESET) {
        if (event == BUTTON_EVENT_LONG) {
            // Check hold duration for factory reset
            screen_factory_reset_check_hold(true);
        } else if (event == BUTTON_EVENT_DOUBLE) {
            // Back to menu
            lv_obj_t *menu_screen = lv_obj_create(NULL);
            screen_menu_create(menu_screen);
            lv_screen_load(menu_screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = menu_screen;
            current_screen_type = UI_SCREEN_MENU;
        }
    } else if (current_screen_type == UI_SCREEN_LORA_PRESETS) {
        if (event == BUTTON_EVENT_SHORT) {
            screen_lora_presets_navigate_down();
            lv_obj_t *screen = lv_obj_create(NULL);
            screen_lora_presets_create(screen);
            lv_screen_load(screen);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = screen;
        } else if (event == BUTTON_EVENT_LONG) {
            screen_lora_presets_select();
            lv_obj_t *submenu = lv_obj_create(NULL);
            screen_lora_submenu_create(submenu);
            lv_screen_load(submenu);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = submenu;
            current_screen_type = UI_SCREEN_LORA_SUBMENU;
        } else if (event == BUTTON_EVENT_DOUBLE) {
            lv_obj_t *submenu = lv_obj_create(NULL);
            screen_lora_submenu_create(submenu);
            lv_screen_load(submenu);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = submenu;
            current_screen_type = UI_SCREEN_LORA_SUBMENU;
        }
    } else if (current_screen_type == UI_SCREEN_LORA_FREQUENCY ||
               current_screen_type == UI_SCREEN_LORA_SF ||
               current_screen_type == UI_SCREEN_LORA_BW) {
        bool is_edit = false;
        
        if (current_screen_type == UI_SCREEN_LORA_FREQUENCY) {
            is_edit = screen_lora_frequency_is_edit_mode();
            if (event == BUTTON_EVENT_SHORT && is_edit) {
                screen_lora_frequency_navigate_down();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_lora_frequency_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_DOUBLE && is_edit) {
                screen_lora_frequency_navigate_up();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_lora_frequency_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_LONG) {
                screen_lora_frequency_select();
                if (!screen_lora_frequency_is_edit_mode()) {
                    // Saved, go back to submenu
                    lv_obj_t *submenu = lv_obj_create(NULL);
                    screen_lora_submenu_create(submenu);
                    lv_screen_load(submenu);
                    if (current_screen) lv_obj_del(current_screen);
                    current_screen = submenu;
                    current_screen_type = UI_SCREEN_LORA_SUBMENU;
                } else {
                    // Entered edit mode, redraw
                    lv_obj_t *screen = lv_obj_create(NULL);
                    screen_lora_frequency_create(screen);
                    lv_screen_load(screen);
                    if (current_screen) lv_obj_del(current_screen);
                    current_screen = screen;
                }
            }
        } else if (current_screen_type == UI_SCREEN_LORA_SF) {
            is_edit = screen_lora_sf_is_edit_mode();
            if (event == BUTTON_EVENT_SHORT && is_edit) {
                screen_lora_sf_navigate_down();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_lora_sf_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_LONG) {
                screen_lora_sf_select();
                if (!screen_lora_sf_is_edit_mode()) {
                    lv_obj_t *submenu = lv_obj_create(NULL);
                    screen_lora_submenu_create(submenu);
                    lv_screen_load(submenu);
                    if (current_screen) lv_obj_del(current_screen);
                    current_screen = submenu;
                    current_screen_type = UI_SCREEN_LORA_SUBMENU;
                } else {
                    lv_obj_t *screen = lv_obj_create(NULL);
                    screen_lora_sf_create(screen);
                    lv_screen_load(screen);
                    if (current_screen) lv_obj_del(current_screen);
                    current_screen = screen;
                }
            }
        } else if (current_screen_type == UI_SCREEN_LORA_BW) {
            is_edit = screen_lora_bw_is_edit_mode();
            if (event == BUTTON_EVENT_SHORT && is_edit) {
                screen_lora_bw_navigate_down();
                lv_obj_t *screen = lv_obj_create(NULL);
                screen_lora_bw_create(screen);
                lv_screen_load(screen);
                if (current_screen) lv_obj_del(current_screen);
                current_screen = screen;
            } else if (event == BUTTON_EVENT_LONG) {
                screen_lora_bw_select();
                if (!screen_lora_bw_is_edit_mode()) {
                    lv_obj_t *submenu = lv_obj_create(NULL);
                    screen_lora_submenu_create(submenu);
                    lv_screen_load(submenu);
                    if (current_screen) lv_obj_del(current_screen);
                    current_screen = submenu;
                    current_screen_type = UI_SCREEN_LORA_SUBMENU;
                } else {
                    lv_obj_t *screen = lv_obj_create(NULL);
                    screen_lora_bw_create(screen);
                    lv_screen_load(screen);
                    if (current_screen) lv_obj_del(current_screen);
                    current_screen = screen;
                }
            }
        }
        
        if (event == BUTTON_EVENT_DOUBLE && !is_edit) {
            // Back to submenu
            lv_obj_t *submenu = lv_obj_create(NULL);
            screen_lora_submenu_create(submenu);
            lv_screen_load(submenu);
            if (current_screen) lv_obj_del(current_screen);
            current_screen = submenu;
            current_screen_type = UI_SCREEN_LORA_SUBMENU;
        }
    }
    
    ui_lvgl_unlock();
}

static void mode_change_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    system_event_mode_t *evt = (system_event_mode_t *)data;
    ESP_LOGI(TAG, "Mode changed to: %s", evt->mode == DEVICE_MODE_PRESENTER ? "PRESENTER" : "PC");
    
    // Reload main screen with new mode
    ui_compact_show_main_screen();
}

esp_err_t ui_compact_init(void) {
    ESP_LOGI(TAG, "Initializing compact UI for small displays");

    // Initialize LVGL core
    esp_err_t ret = ui_lvgl_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL core");
        return ret;
    }

    // Initialize UI component styles
    ui_components_init();

    // Register for mode change events
    esp_event_loop_handle_t loop = system_events_get_loop();
    esp_event_handler_register_with(loop, SYSTEM_EVENTS, SYSTEM_EVENT_MODE_CHANGED,
                                    mode_change_event_handler, NULL);

    ESP_LOGI(TAG, "Compact UI initialized");
    return ESP_OK;
}

esp_err_t ui_compact_show_boot_screen(void) {
    ui_lvgl_lock();
    
    lv_obj_t *screen = lv_obj_create(NULL);
    screen_boot_create(screen);
    lv_screen_load(screen);
    
    if (current_screen) lv_obj_del(current_screen);
    current_screen = screen;
    current_screen_type = UI_SCREEN_BOOT;
    
    ui_lvgl_unlock();
    return ESP_OK;
}

esp_err_t ui_compact_show_main_screen(void) {
    ui_lvgl_lock();
    
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    
    statusbar_data_t status;
    get_current_status(&status);
    
    // Check device mode and show appropriate screen
    general_config_t config;
    general_config_get(&config);
    
    if (config.device_mode == DEVICE_MODE_PC) {
        screen_pc_mode_create(screen, &status);
    } else {
        screen_main_create(screen, &status);
    }
    
    lv_screen_load(screen);
    
    if (current_screen) lv_obj_del(current_screen);
    current_screen = screen;
    current_screen_type = UI_SCREEN_MAIN;
    
    ui_lvgl_unlock();
    return ESP_OK;
}

esp_err_t ui_compact_register_button_callback(void) {
    button_manager_register_callback(button_event_handler, NULL);
    ESP_LOGI(TAG, "Button callback registered");
    return ESP_OK;
}

// Stub for commands component
esp_err_t ui_data_provider_reload_config(void) {
    ESP_LOGI(TAG, "Config reload requested (stub)");
    return ESP_OK;
}
