#include "ui_screen_controller.h"
#include "ui_data_provider.h"
#include "boot_screen.h"
#include "main_screen.h"
#include "info_screens.h"
#include "menu_screen.h"
#include "esp_log.h"

static const char* TAG = "ui_screen_controller";
static oled_screen_t current_screen = OLED_SCREEN_BOOT;

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
            
        case OLED_SCREEN_DEVICE_INFO:
            device_info_screen_draw(status);
            break;
            
        default:
            ESP_LOGW(TAG, "Screen %d not implemented, showing main", screen);
            main_screen_draw(status);
            current_screen = OLED_SCREEN_MAIN;
            break;
    }
}

void ui_screen_controller_update(const ui_status_t* status) {
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
            if (button == OLED_BUTTON_PREV) {
                menu_screen_navigate(MENU_UP);
                menu_screen_draw();
            } else if (button == OLED_BUTTON_NEXT) {
                menu_screen_navigate(MENU_DOWN);
                menu_screen_draw();
            } else if (button == OLED_BUTTON_BOTH) {
                // Select menu item
                int selected = menu_screen_get_selected();
                switch (selected) {
                    case MENU_DEVICE_INFO:
                        ui_screen_controller_set(OLED_SCREEN_DEVICE_INFO, NULL);
                        break;
                    case MENU_SYSTEM_INFO:
                        ui_screen_controller_set(OLED_SCREEN_SYSTEM_INFO, NULL);
                        break;
                    default:
                        ESP_LOGI(TAG, "Menu item %d not implemented yet", selected);
                        break;
                }
            }
            break;
            
        case OLED_SCREEN_SYSTEM_INFO:
        case OLED_SCREEN_DEVICE_INFO:
            if (button == OLED_BUTTON_PREV) {
                // Back to menu
                ui_screen_controller_set(OLED_SCREEN_MENU, NULL);
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Button handling not implemented for screen %d", current_screen);
            break;
    }
}
