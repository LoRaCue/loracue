#include "ui_screen_controller.h"
#include "ui_data_provider.h"
#include "boot_screen.h"
#include "main_screen.h"
#include "info_screens.h"
#include "menu_screen.h"
#include "device_mode_screen.h"
#include "lora_settings_screen.h"
#include "pairing_screen.h"
#include "device_registry_screen.h"
#include "brightness_screen.h"
#include "config_mode_screen.h"
#include "factory_reset_screen.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "ui_screen_controller";
static oled_screen_t current_screen = OLED_SCREEN_BOOT;
static uint32_t menu_enter_time = 0;

#define MENU_TIMEOUT_MS 15000  // 15 seconds

void ui_screen_controller_init(void) {
    ESP_LOGI(TAG, "Initializing screen controller");
    
    // Initialize data provider
    esp_err_t ret = ui_data_provider_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize data provider: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "Screen controller initialized");
}

void ui_screen_controller_set(oled_screen_t screen, const ui_status_t* status) {
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
            
        case OLED_SCREEN_MENU:
            menu_screen_draw();
            break;
            
        case OLED_SCREEN_SYSTEM_INFO:
            system_info_screen_draw();
            break;
            
        case OLED_SCREEN_FACTORY_RESET:
            factory_reset_screen_draw();
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
            
        case OLED_SCREEN_LORA_SETTINGS:
            lora_settings_screen_draw();
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
            
        case OLED_SCREEN_CONFIG_ACTIVE:
            config_mode_screen_draw();
            break;
            
        default:
            ESP_LOGW(TAG, "Screen %d not implemented, showing main", screen);
            main_screen_draw(status);
            current_screen = OLED_SCREEN_MAIN;
            break;
    }
}

oled_screen_t ui_screen_controller_get_current(void) {
    return current_screen;
}

void ui_screen_controller_update(const ui_status_t* status) {
    // Check menu timeout
    if (current_screen == OLED_SCREEN_MENU) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (current_time - menu_enter_time >= MENU_TIMEOUT_MS) {
            ESP_LOGI(TAG, "Menu timeout - returning to main screen");
            ui_screen_controller_set(OLED_SCREEN_MAIN, NULL);
            return;
        }
    }
    
    // Force update data provider if status provided
    if (status) {
        ui_data_provider_force_update(status->usb_connected, status->lora_connected, status->battery_level);
    } else {
        // Update from BSP sensors
        esp_err_t ret = ui_data_provider_update();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to update data: %s", esp_err_to_name(ret));
        }
    }
    
    // Redraw current screen with updated data
    ui_screen_controller_set(current_screen, NULL);
}

void ui_screen_controller_handle_button(oled_button_t button, bool long_press) {
    ESP_LOGI(TAG, "Button %d pressed (long: %d) on screen %d", button, long_press, current_screen);
    
    switch (current_screen) {
        case OLED_SCREEN_BOOT:
            // Boot screen auto-transitions, ignore buttons
            break;
            
        case OLED_SCREEN_MAIN:
            if (long_press && button == OLED_BUTTON_BOTH) {
                // Long press both buttons -> Menu
                menu_screen_reset();
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            }
            break;
            
        case OLED_SCREEN_MENU:
            if (long_press && button == OLED_BUTTON_BOTH) {
                // Long press both buttons -> Exit menu
                ESP_LOGI(TAG, "Exiting menu - returning to main screen");
                ui_screen_controller_set(OLED_SCREEN_MAIN, NULL);
            } else if (button == OLED_BUTTON_PREV) {
                menu_screen_navigate(MENU_UP);
                menu_screen_draw();
            } else if (button == OLED_BUTTON_NEXT) {
                menu_screen_navigate(MENU_DOWN);
                menu_screen_draw();
            } else if (button == OLED_BUTTON_BOTH) {
                // Select menu item
                int selected = menu_screen_get_selected();
                switch (selected) {
                    case MENU_DEVICE_MODE:
                        device_mode_screen_reset();
                        ui_screen_controller_set(OLED_SCREEN_DEVICE_MODE, NULL);
                        break;
                    case MENU_LORA_SETTINGS:
                        lora_settings_screen_reset();
                        ui_screen_controller_set(OLED_SCREEN_LORA_SETTINGS, NULL);
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
            if (button == OLED_BUTTON_PREV) {
                // Back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            }
            break;
            
        case OLED_SCREEN_FACTORY_RESET:
            if (button == OLED_BUTTON_PREV) {
                // Back to menu
                factory_reset_screen_select();
            } else if (long_press && button == OLED_BUTTON_BOTH) {
                // Long press both buttons (5s) - execute factory reset
                factory_reset_screen_execute();
            }
            break;
            
        case OLED_SCREEN_DEVICE_MODE:
            if (button == OLED_BUTTON_PREV) {
                // Back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            } else if (button == OLED_BUTTON_NEXT) {
                device_mode_screen_navigate(MENU_DOWN);
                device_mode_screen_draw();
            } else if (button == OLED_BUTTON_BOTH) {
                device_mode_screen_select();
                device_mode_screen_draw();
            }
            break;
            
        case OLED_SCREEN_LORA_SETTINGS:
            if (button == OLED_BUTTON_PREV) {
                // Back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            } else if (button == OLED_BUTTON_NEXT) {
                lora_settings_screen_navigate(MENU_DOWN);
                lora_settings_screen_draw();
            } else if (button == OLED_BUTTON_BOTH) {
                lora_settings_screen_select();
                lora_settings_screen_draw();
            }
            break;
            
        case OLED_SCREEN_BRIGHTNESS:
            if (button == OLED_BUTTON_PREV) {
                if (brightness_screen_is_edit_mode()) {
                    brightness_screen_navigate(MENU_UP);
                    brightness_screen_draw();
                } else {
                    ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
                }
            } else if (button == OLED_BUTTON_NEXT) {
                if (brightness_screen_is_edit_mode()) {
                    brightness_screen_navigate(MENU_DOWN);
                    brightness_screen_draw();
                }
            } else if (button == OLED_BUTTON_BOTH) {
                brightness_screen_select();
                brightness_screen_draw();
            }
            break;
            
        case OLED_SCREEN_DEVICE_PAIRING:
            if (button == OLED_BUTTON_PREV) {
                // Stop pairing and back to menu
                // TODO: Stop LoRa pairing instead of USB pairing
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            }
            break;
            
        case OLED_SCREEN_DEVICE_REGISTRY:
            if (button == OLED_BUTTON_PREV) {
                // Back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            } else if (button == OLED_BUTTON_NEXT) {
                device_registry_screen_navigate(MENU_DOWN);
                device_registry_screen_draw();
            } else if (button == OLED_BUTTON_BOTH) {
                // TODO: Implement device removal
                ESP_LOGI(TAG, "Device removal not implemented yet");
            }
            break;
            
        case OLED_SCREEN_CONFIG_ACTIVE:
            if (button == OLED_BUTTON_PREV) {
                // Back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            } else if (button == OLED_BUTTON_BOTH) {
                // Both buttons -> Toggle display mode
                config_mode_screen_toggle_display();
                config_mode_screen_draw();
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Button handling not implemented for screen %d", current_screen);
            break;
    }
}
