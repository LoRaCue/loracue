#include "ui_screen_controller.h"
extern void ui_screen_ota_update(void);
#include "bluetooth_screen.h"
#include "boot_screen.h"
#include "brightness_screen.h"
#include "config_mode_screen.h"
#include "config_wifi_server.h"
#include "device_mode_screen.h"
#include "device_registry_screen.h"
#include "esp_log.h"
#include "factory_reset_screen.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "general_config.h"
#include "info_screens.h"
#include "lora_band_screen.h"
#include "lora_bw_screen.h"
#include "lora_cr_screen.h"
#include "lora_frequency_screen.h"
#include "lora_protocol.h"
#include "lora_settings_screen.h"
#include "lora_sf_screen.h"
#include "lora_submenu_screen.h"
#include "lora_txpower_screen.h"
#include "main_screen.h"
#include "menu_screen.h"
#include "pairing_screen.h"
#include "pc_mode_screen.h"
#include "slot_screen.h"
#include "ui_data_provider.h"

static const char *TAG              = "ui_screen_controller";
static ui_mini_screen_t current_screen = OLED_SCREEN_BOOT;
static uint32_t menu_enter_time     = 0;

#define MENU_TIMEOUT_MS 15000 // 15 seconds

void ui_screen_controller_init(void)
{
    ESP_LOGI(TAG, "Initializing screen controller");

    // Initialize data provider
    esp_err_t ret = ui_data_provider_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize data provider: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Screen controller initialized");
}

void ui_screen_controller_set(ui_mini_screen_t screen, const ui_status_t *status)
{
    // Acquire draw lock
    extern bool ui_mini_try_lock_draw(void);
    extern void ui_mini_unlock_draw(void);

    if (!ui_mini_try_lock_draw()) {
        ESP_LOGW(TAG, "Failed to acquire draw lock, skipping screen change");
        return;
    }

    current_screen = screen;

    // Track menu entry time for timeout
    if (screen == OLED_SCREEN_MENU) {
        menu_enter_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }

    // Update data if needed
    if (!status) {
        ui_data_provider_update();
        status = ui_data_provider_get_status();
    }

    if (!status) {
        ESP_LOGE(TAG, "No status data available");
        ui_mini_unlock_draw(); // Release lock before returning
        return;
    }

    ESP_LOGI(TAG, "Setting screen: %d", screen);

    switch (screen) {
        case OLED_SCREEN_BOOT:
            boot_screen_draw();
            break;

        case OLED_SCREEN_MAIN:
            main_screen_draw(status);
            break;

        case OLED_SCREEN_PC_MODE:
            // PC mode needs ui_mini_status_t for command history
            {
                extern ui_mini_status_t g_oled_status;
                // Check if g_oled_status is initialized (device_name will be set)
                if (g_oled_status.device_name[0] != '\0') {
                    pc_mode_screen_draw(&g_oled_status);
                } else {
                    // Not initialized yet, draw empty PC mode screen
                    ui_mini_status_t temp_status  = {0};
                    temp_status.battery_level  = status->battery_level;
                    temp_status.usb_connected  = status->usb_connected;
                    temp_status.lora_connected = status->lora_connected;
                    pc_mode_screen_draw(&temp_status);
                }
            }
            break;

        case OLED_SCREEN_MENU:
            menu_screen_draw();
            break;

        case OLED_SCREEN_SYSTEM_INFO:
            system_info_screen_draw();
            break;

        case OLED_SCREEN_FACTORY_RESET:
            factory_reset_screen_draw();
            break;

        case OLED_SCREEN_OTA_UPDATE:
            ui_screen_ota_update();
            break;

        case OLED_SCREEN_DEVICE_INFO:
            device_info_screen_draw(status);
            break;

        case OLED_SCREEN_BATTERY:
            battery_status_screen_draw(status);
            break;

        case OLED_SCREEN_DEVICE_MODE:
            device_mode_screen_draw();
            break;

        case OLED_SCREEN_LORA_SUBMENU:
            lora_submenu_screen_draw();
            break;

        case OLED_SCREEN_LORA_SETTINGS:
            lora_settings_screen_draw();
            break;

        case OLED_SCREEN_LORA_FREQUENCY:
            lora_frequency_screen_draw();
            break;

        case OLED_SCREEN_LORA_SF:
            lora_sf_screen_draw();
            break;

        case OLED_SCREEN_LORA_BW:
            lora_bw_screen_draw();
            break;

        case OLED_SCREEN_LORA_CR:
            lora_cr_screen_draw();
            break;

        case OLED_SCREEN_LORA_TXPOWER:
            lora_txpower_screen_draw();
            break;

        case OLED_SCREEN_LORA_BAND:
            lora_band_screen_draw();
            break;

        case OLED_SCREEN_SLOT:
            slot_screen_draw();
            break;

        case OLED_SCREEN_DEVICE_PAIRING:
            pairing_screen_draw();
            break;

        case OLED_SCREEN_DEVICE_REGISTRY:
            device_registry_screen_draw();
            break;

        case OLED_SCREEN_BRIGHTNESS:
            brightness_screen_draw();
            break;

        case OLED_SCREEN_BLUETOOTH:
            bluetooth_screen_draw();
            break;

        case OLED_SCREEN_CONFIG_ACTIVE:
            config_mode_screen_draw();
            break;

        default:
            ESP_LOGW(TAG, "Screen %d not implemented, showing main", screen);
            main_screen_draw(status);
            current_screen = OLED_SCREEN_MAIN;
            break;
    }

    // Release draw lock
    ui_mini_unlock_draw();
}

ui_mini_screen_t ui_screen_controller_get_current(void)
{
    return current_screen;
}

void ui_screen_controller_set_no_draw(ui_mini_screen_t screen)
{
    current_screen = screen;
    ESP_LOGI(TAG, "Screen type changed to: %d (no draw)", screen);
}

void ui_screen_controller_update(const ui_status_t *status)
{
    // Check menu timeout
    if (current_screen == OLED_SCREEN_MENU) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (current_time - menu_enter_time >= MENU_TIMEOUT_MS) {
            ESP_LOGI(TAG, "Menu timeout - returning to main screen");
            ui_screen_controller_set(OLED_SCREEN_MAIN, NULL);
            return;
        }
    }

    // Update data provider if status not provided
    if (!status) {
        esp_err_t ret = ui_data_provider_update();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to update data: %s", esp_err_to_name(ret));
            return;
        }
        status = ui_data_provider_get_status();
    } else {
        ui_data_provider_force_update(status->usb_connected, status->lora_connected, status->battery_level);
    }

    if (!status) {
        ESP_LOGW(TAG, "No status data available for update");
        return;
    }

    // Redraw current screen with updated data (only screens that need periodic updates)
    switch (current_screen) {
        case OLED_SCREEN_MAIN:
            main_screen_draw(status);
            break;

        case OLED_SCREEN_PC_MODE: {
            extern ui_mini_status_t g_oled_status;
            pc_mode_screen_draw(&g_oled_status);
        } break;

        case OLED_SCREEN_BATTERY:
            battery_status_screen_draw(status);
            break;

        // Other screens don't need periodic updates
        default:
            break;
    }
}

void ui_screen_controller_handle_button(button_event_type_t event)
{
    ESP_LOGI(TAG, "Button event %d on screen %d", event, current_screen);

    // Reset menu timeout on any button press while in menu
    if (current_screen == OLED_SCREEN_MENU) {
        menu_enter_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }

    switch (current_screen) {
        case OLED_SCREEN_BOOT:
            break;

        case OLED_SCREEN_MAIN: {
            extern device_mode_t current_device_mode;
            
            if (current_device_mode == DEVICE_MODE_PC) {
                // PC mode: send HID directly via USB
                if (event == BUTTON_EVENT_SHORT) {
                    extern esp_err_t usb_hid_send_key(uint8_t keycode);
                    usb_hid_send_key(0x4E); // Page Down
                } else if (event == BUTTON_EVENT_DOUBLE) {
                    extern esp_err_t usb_hid_send_key(uint8_t keycode);
                    usb_hid_send_key(0x4B); // Page Up
                } else if (event == BUTTON_EVENT_LONG) {
                    menu_screen_reset();
                    ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
                }
            } else {
                // Presenter mode: send via LoRa
                if (event == BUTTON_EVENT_SHORT) {
                    general_config_t config;
                    general_config_get(&config);
#ifdef CONFIG_LORA_SEND_RELIABLE
                    lora_protocol_send_keyboard_reliable(config.slot_id, 0, 0x4E, 2000, 3); // Page Down
#else
                    lora_protocol_send_keyboard(config.slot_id, 0, 0x4E); // Page Down (unreliable)
#endif
                } else if (event == BUTTON_EVENT_DOUBLE) {
                    general_config_t config;
                    general_config_get(&config);
#ifdef CONFIG_LORA_SEND_RELIABLE
                    lora_protocol_send_keyboard_reliable(config.slot_id, 0, 0x4B, 2000, 3); // Page Up
#else
                    lora_protocol_send_keyboard(config.slot_id, 0, 0x4B); // Page Up (unreliable)
#endif
                } else if (event == BUTTON_EVENT_LONG) {
                    menu_screen_reset();
                    ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
                }
            }
            break;
        }

        case OLED_SCREEN_PC_MODE:
            if (event == BUTTON_EVENT_LONG) {
                menu_screen_reset();
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            }
            break;

        case OLED_SCREEN_MENU:
            if (event == BUTTON_EVENT_SHORT) {
                menu_screen_navigate(MENU_DOWN);
                menu_screen_draw();
            } else if (event == BUTTON_EVENT_DOUBLE) {
                // Double press = back to main screen
                ui_screen_controller_set(OLED_SCREEN_MAIN, NULL);
            } else if (event == BUTTON_EVENT_LONG) {
                int selected = menu_screen_get_selected();
                switch (selected) {
                    case MENU_DEVICE_MODE:
                        device_mode_screen_reset();
                        ui_screen_controller_set(OLED_SCREEN_DEVICE_MODE, NULL);
                        break;
                    case MENU_SLOT:
                        slot_screen_init();
                        ui_screen_controller_set(OLED_SCREEN_SLOT, NULL);
                        break;
                    case MENU_LORA_SETTINGS:
                        ui_screen_controller_set(OLED_SCREEN_LORA_SUBMENU, NULL);
                        break;
                    case MENU_DEVICE_PAIRING:
                        pairing_screen_reset();
                        ui_screen_controller_set(OLED_SCREEN_DEVICE_PAIRING, NULL);
                        break;
                    case MENU_DEVICE_REGISTRY:
                        device_registry_screen_reset();
                        ui_screen_controller_set(OLED_SCREEN_DEVICE_REGISTRY, NULL);
                        break;
                    case MENU_BRIGHTNESS:
                        brightness_screen_init();
                        ui_screen_controller_set(OLED_SCREEN_BRIGHTNESS, NULL);
                        break;
                    case MENU_BLUETOOTH:
                        ui_screen_controller_set(OLED_SCREEN_BLUETOOTH, NULL);
                        break;
                    case MENU_CONFIG_MODE:
                        config_mode_screen_reset();
                        ui_screen_controller_set(OLED_SCREEN_CONFIG_ACTIVE, NULL);
                        break;
                    case MENU_BATTERY_STATUS:
                        ui_screen_controller_set(OLED_SCREEN_BATTERY, NULL);
                        break;
                    case MENU_DEVICE_INFO:
                        ui_screen_controller_set(OLED_SCREEN_DEVICE_INFO, NULL);
                        break;
                    case MENU_SYSTEM_INFO:
                        ui_screen_controller_set(OLED_SCREEN_SYSTEM_INFO, NULL);
                        break;
                    case MENU_FACTORY_RESET:
                        ui_screen_controller_set(OLED_SCREEN_FACTORY_RESET, NULL);
                        break;
                    default:
                        ESP_LOGI(TAG, "Menu item %d not implemented yet", selected);
                        break;
                }
            }
            break;

        case OLED_SCREEN_SYSTEM_INFO:
        case OLED_SCREEN_DEVICE_INFO:
        case OLED_SCREEN_BATTERY:
        case OLED_SCREEN_DEVICE_PAIRING:
        case OLED_SCREEN_DEVICE_REGISTRY:
            if (event == BUTTON_EVENT_DOUBLE) {
                // Double press = back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            }
            break;

        case OLED_SCREEN_FACTORY_RESET:
            if (event == BUTTON_EVENT_DOUBLE) {
                // Double press = back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            } else if (event == BUTTON_EVENT_LONG) {
                factory_reset_screen_execute();
            }
            break;

        case OLED_SCREEN_DEVICE_MODE:
            if (event == BUTTON_EVENT_SHORT) {
                device_mode_screen_navigate(MENU_DOWN);
                device_mode_screen_draw();
            } else if (event == BUTTON_EVENT_DOUBLE) {
                // Double press = back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            } else if (event == BUTTON_EVENT_LONG) {
                device_mode_screen_select();
            }
            break;

        case OLED_SCREEN_LORA_SUBMENU:
            if (event == BUTTON_EVENT_SHORT) {
                lora_submenu_screen_navigate(MENU_DOWN);
                lora_submenu_screen_draw();
            } else if (event == BUTTON_EVENT_DOUBLE) {
                // Double press = back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            } else if (event == BUTTON_EVENT_LONG) {
                lora_submenu_screen_select();
            }
            break;

        case OLED_SCREEN_LORA_SETTINGS:
            if (event == BUTTON_EVENT_SHORT) {
                lora_settings_screen_navigate(MENU_DOWN);
                lora_settings_screen_draw();
            } else if (event == BUTTON_EVENT_DOUBLE) {
                // Double press = back to submenu
                ui_screen_controller_set(OLED_SCREEN_LORA_SUBMENU, NULL);
            } else if (event == BUTTON_EVENT_LONG) {
                lora_settings_screen_select();
                lora_settings_screen_draw();
            }
            break;

        case OLED_SCREEN_LORA_FREQUENCY:
            if (lora_frequency_screen_is_edit_mode()) {
                if (event == BUTTON_EVENT_SHORT) {
                    lora_frequency_screen_navigate(MENU_DOWN);
                    lora_frequency_screen_draw();
                } else if (event == BUTTON_EVENT_DOUBLE) {
                    lora_frequency_screen_navigate(MENU_UP);
                    lora_frequency_screen_draw();
                } else if (event == BUTTON_EVENT_LONG) {
                    lora_frequency_screen_select();
                    lora_frequency_screen_draw();
                }
            } else {
                if (event == BUTTON_EVENT_DOUBLE) {
                    // Double press = back to submenu
                    ui_screen_controller_set(OLED_SCREEN_LORA_SUBMENU, NULL);
                } else if (event == BUTTON_EVENT_LONG) {
                    lora_frequency_screen_select();
                    lora_frequency_screen_draw();
                }
            }
            break;

        case OLED_SCREEN_LORA_SF:
        case OLED_SCREEN_LORA_BW:
        case OLED_SCREEN_LORA_CR:
        case OLED_SCREEN_LORA_TXPOWER:
        case OLED_SCREEN_LORA_BAND:
            if (event == BUTTON_EVENT_SHORT) {
                if (current_screen == OLED_SCREEN_LORA_SF)
                    lora_sf_screen_navigate(MENU_DOWN);
                else if (current_screen == OLED_SCREEN_LORA_BW)
                    lora_bw_screen_navigate(MENU_DOWN);
                else if (current_screen == OLED_SCREEN_LORA_CR)
                    lora_cr_screen_navigate(MENU_DOWN);
                else if (current_screen == OLED_SCREEN_LORA_TXPOWER)
                    lora_txpower_screen_navigate(MENU_DOWN);
                else if (current_screen == OLED_SCREEN_LORA_BAND)
                    lora_band_screen_navigate(MENU_DOWN);

                if (current_screen == OLED_SCREEN_LORA_SF)
                    lora_sf_screen_draw();
                else if (current_screen == OLED_SCREEN_LORA_BW)
                    lora_bw_screen_draw();
                else if (current_screen == OLED_SCREEN_LORA_CR)
                    lora_cr_screen_draw();
                else if (current_screen == OLED_SCREEN_LORA_TXPOWER)
                    lora_txpower_screen_draw();
                else if (current_screen == OLED_SCREEN_LORA_BAND)
                    lora_band_screen_draw();
            } else if (event == BUTTON_EVENT_DOUBLE) {
                // Double press = back to submenu
                ui_screen_controller_set(OLED_SCREEN_LORA_SUBMENU, NULL);
            } else if (event == BUTTON_EVENT_LONG) {
                if (current_screen == OLED_SCREEN_LORA_SF) {
                    lora_sf_screen_select();
                    ui_screen_controller_set(OLED_SCREEN_LORA_SUBMENU, NULL);
                } else if (current_screen == OLED_SCREEN_LORA_BW) {
                    lora_bw_screen_select();
                    ui_screen_controller_set(OLED_SCREEN_LORA_SUBMENU, NULL);
                } else if (current_screen == OLED_SCREEN_LORA_CR) {
                    lora_cr_screen_select();
                    ui_screen_controller_set(OLED_SCREEN_LORA_SUBMENU, NULL);
                } else if (current_screen == OLED_SCREEN_LORA_TXPOWER) {
                    lora_txpower_screen_select();
                    ui_screen_controller_set(OLED_SCREEN_LORA_SUBMENU, NULL);
                } else if (current_screen == OLED_SCREEN_LORA_BAND) {
                    lora_band_screen_select();
                    ui_screen_controller_set(OLED_SCREEN_LORA_SUBMENU, NULL);
                }
            }
            break;

        case OLED_SCREEN_SLOT:
            if (slot_screen_is_edit_mode()) {
                if (event == BUTTON_EVENT_SHORT) {
                    slot_screen_navigate(MENU_DOWN);
                    slot_screen_draw();
                } else if (event == BUTTON_EVENT_DOUBLE) {
                    slot_screen_navigate(MENU_UP);
                    slot_screen_draw();
                } else if (event == BUTTON_EVENT_LONG) {
                    slot_screen_select();
                }
            } else {
                if (event == BUTTON_EVENT_DOUBLE) {
                    // Double press = back to main menu (slot is accessed from main menu, not submenu)
                    ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
                } else if (event == BUTTON_EVENT_LONG) {
                    slot_screen_select();
                    slot_screen_draw();
                }
            }
            break;

        case OLED_SCREEN_BRIGHTNESS:
            if (brightness_screen_is_edit_mode()) {
                if (event == BUTTON_EVENT_SHORT) {
                    brightness_screen_navigate(MENU_DOWN);
                    brightness_screen_draw();
                } else if (event == BUTTON_EVENT_DOUBLE) {
                    brightness_screen_navigate(MENU_UP);
                    brightness_screen_draw();
                } else if (event == BUTTON_EVENT_LONG) {
                    brightness_screen_select();                       // Saves and exits edit mode
                    ui_screen_controller_set(OLED_SCREEN_MENU, NULL); // Return to menu
                }
            } else {
                if (event == BUTTON_EVENT_DOUBLE) {
                    // Double press = back to menu
                    ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
                } else if (event == BUTTON_EVENT_LONG) {
                    brightness_screen_select(); // Enter edit mode
                    brightness_screen_draw();
                }
            }
            break;

        case OLED_SCREEN_BLUETOOTH:
            if (event == BUTTON_EVENT_SHORT) {
                bluetooth_screen_handle_input(1);
            } else if (event == BUTTON_EVENT_DOUBLE) {
                // Double press = back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            } else if (event == BUTTON_EVENT_LONG) {
                bluetooth_screen_handle_input(2);
            }
            break;

        case OLED_SCREEN_CONFIG_ACTIVE:
            if (event == BUTTON_EVENT_SHORT) {
                config_mode_screen_toggle_display();
                config_mode_screen_draw();
            } else if (event == BUTTON_EVENT_DOUBLE) {
                // Double press = back to menu
                config_wifi_server_stop();
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            } else if (event == BUTTON_EVENT_LONG) {
                config_wifi_server_stop();
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            }
            break;

        default:
            ESP_LOGW(TAG, "Button handling not implemented for screen %d", current_screen);
            break;
    }
}
