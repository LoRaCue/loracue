/**
 * @file oled_ui.c
 * @brief OLED UI implementation with hierarchical menu system
 * 
 * CONTEXT: Two-button navigation following blueprint Section 3.4.1
 * NAVIGATION: PREV=Up/Back, NEXT=Down/Select, BOTH=Menu, LONG=Special
 */

#include "oled_ui.h"
#include "bsp.h"
#include "power_mgmt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "OLED_UI";

// UI State
static ui_state_t current_state = UI_STATE_BOOT_LOGO;
static device_status_t current_status = {0};
static int menu_selection = 0;
static bool ui_initialized = false;

// Menu definitions
static const char* main_menu_items[] = {
    "Wireless",
    "Pairing", 
    "System",
    "Back"
};
#define MAIN_MENU_COUNT (sizeof(main_menu_items) / sizeof(main_menu_items[0]))

static const char* wireless_menu_items[] = {
    "Signal: Auto",
    "Power: High", 
    "Channel: 915MHz",
    "Back"
};
#define WIRELESS_MENU_COUNT (sizeof(wireless_menu_items) / sizeof(wireless_menu_items[0]))

static const char* pairing_menu_items[] = {
    "Pair New Device",
    "Show Pairing PIN",
    "Paired Devices: 0",
    "Clear All Pairs",
    "Back"
};
#define PAIRING_MENU_COUNT (sizeof(pairing_menu_items) / sizeof(pairing_menu_items[0]))

static const char* system_menu_items[] = {
    "Device Name",
    "Sleep: 5min",
    "Web Config",
    "Reset Settings",
    "Back"
};
#define SYSTEM_MENU_COUNT (sizeof(system_menu_items) / sizeof(system_menu_items[0]))

// Display functions
static void draw_text_line(int line, const char* text, bool inverted)
{
    // Placeholder - would implement actual SH1106 text rendering
    ESP_LOGI(TAG, "Line %d: %s%s", line, inverted ? "[*] " : "[ ] ", text);
}

static void clear_display(void)
{
    bsp_oled_clear();
    ESP_LOGD(TAG, "Display cleared");
}

static void draw_main_screen(void)
{
    clear_display();
    
    char line1[32], line2[32], line3[32], line4[32];
    
    // Line 1: Device name
    snprintf(line1, sizeof(line1), "%.16s", current_status.device_name[0] ? current_status.device_name : "LoRaCue-STAGE");
    
    // Line 2: Battery and signal
    int battery_percent = (int)((current_status.battery_voltage - 3.0f) / 1.2f * 100);
    if (battery_percent > 100) battery_percent = 100;
    if (battery_percent < 0) battery_percent = 0;
    
    snprintf(line2, sizeof(line2), "Bat:%d%% Sig:%d%%", battery_percent, current_status.signal_strength);
    
    // Line 3: Status
    if (current_status.usb_connected) {
        snprintf(line3, sizeof(line3), "USB Connected");
    } else {
        snprintf(line3, sizeof(line3), "Paired: %d devices", current_status.paired_count);
    }
    
    // Line 4: Instructions
    snprintf(line4, sizeof(line4), "Hold both for menu");
    
    draw_text_line(0, line1, false);
    draw_text_line(1, line2, false);
    draw_text_line(2, line3, false);
    draw_text_line(3, line4, false);
}

static void draw_menu(const char** items, int count, int selection, const char* title)
{
    clear_display();
    
    // Title
    draw_text_line(0, title, false);
    
    // Menu items (show 3 at a time)
    int start_idx = (selection > 1) ? selection - 1 : 0;
    for (int i = 0; i < 3 && (start_idx + i) < count; i++) {
        bool selected = (start_idx + i) == selection;
        draw_text_line(i + 1, items[start_idx + i], selected);
    }
}

static void draw_boot_logo(void)
{
    clear_display();
    draw_text_line(1, "   LoRaCue", false);
    draw_text_line(2, "STAGE Remote", false);
    draw_text_line(3, "  Starting...", false);
}

static void draw_pairing_pin(const char* pin)
{
    clear_display();
    draw_text_line(0, "USB Pairing Mode", false);
    draw_text_line(1, "", false);
    
    char pin_line[32];
    snprintf(pin_line, sizeof(pin_line), "PIN: %s", pin ? pin : "------");
    draw_text_line(2, pin_line, false);
    draw_text_line(3, "Enter PIN on PC", false);
}

static void draw_web_config(const char* ssid, const char* ip)
{
    clear_display();
    draw_text_line(0, "Web Config Active", false);
    
    char ssid_line[32];
    snprintf(ssid_line, sizeof(ssid_line), "SSID: %.16s", ssid ? ssid : "LoRaCue-Config");
    draw_text_line(1, ssid_line, false);
    
    char ip_line[32];
    snprintf(ip_line, sizeof(ip_line), "IP: %s", ip ? ip : "192.168.4.1");
    draw_text_line(2, ip_line, false);
    
    draw_text_line(3, "Open in browser", false);
}

static void draw_screensaver(void)
{
    clear_display();
    draw_text_line(2, "Press any button", false);
}

// State machine handlers
static void handle_main_state(ui_event_t event)
{
    switch (event) {
        case UI_EVENT_BOTH_LONG:
            current_state = UI_STATE_MENU;
            menu_selection = 0;
            ESP_LOGI(TAG, "Entering main menu");
            break;
            
        case UI_EVENT_TIMEOUT:
            current_state = UI_STATE_SCREENSAVER;
            ESP_LOGI(TAG, "Entering screensaver");
            break;
            
        default:
            // Ignore other events in main state
            break;
    }
}

static void handle_menu_state(ui_event_t event)
{
    switch (event) {
        case UI_EVENT_PREV_SHORT:
            menu_selection = (menu_selection > 0) ? menu_selection - 1 : MAIN_MENU_COUNT - 1;
            break;
            
        case UI_EVENT_NEXT_SHORT:
            menu_selection = (menu_selection + 1) % MAIN_MENU_COUNT;
            break;
            
        case UI_EVENT_NEXT_LONG:
        case UI_EVENT_BOTH_LONG:
            // Select current menu item
            switch (menu_selection) {
                case 0: // Wireless
                    current_state = UI_STATE_WIRELESS;
                    menu_selection = 0;
                    ESP_LOGI(TAG, "Entering wireless menu");
                    break;
                case 1: // Pairing
                    current_state = UI_STATE_PAIRING;
                    menu_selection = 0;
                    ESP_LOGI(TAG, "Entering pairing menu");
                    break;
                case 2: // System
                    current_state = UI_STATE_SYSTEM;
                    menu_selection = 0;
                    ESP_LOGI(TAG, "Entering system menu");
                    break;
                case 3: // Back
                    current_state = UI_STATE_MAIN;
                    ESP_LOGI(TAG, "Returning to main screen");
                    break;
            }
            break;
            
        case UI_EVENT_PREV_LONG:
            current_state = UI_STATE_MAIN;
            ESP_LOGI(TAG, "Back to main screen");
            break;
            
        default:
            break;
    }
}

static void handle_submenu_state(ui_event_t event, const char** items, int count, ui_state_t parent_state)
{
    switch (event) {
        case UI_EVENT_PREV_SHORT:
            menu_selection = (menu_selection > 0) ? menu_selection - 1 : count - 1;
            break;
            
        case UI_EVENT_NEXT_SHORT:
            menu_selection = (menu_selection + 1) % count;
            break;
            
        case UI_EVENT_NEXT_LONG:
        case UI_EVENT_BOTH_LONG:
            if (menu_selection == count - 1) { // Back option
                current_state = UI_STATE_MENU;
                menu_selection = 0;
                ESP_LOGI(TAG, "Returning to main menu");
            } else {
                ESP_LOGI(TAG, "Selected: %s", items[menu_selection]);
                // Handle specific menu actions here
            }
            break;
            
        case UI_EVENT_PREV_LONG:
            current_state = UI_STATE_MENU;
            menu_selection = 0;
            ESP_LOGI(TAG, "Back to main menu");
            break;
            
        default:
            break;
    }
}

static void handle_screensaver_state(ui_event_t event)
{
    if (event != UI_EVENT_NONE && event != UI_EVENT_TIMEOUT) {
        current_state = UI_STATE_MAIN;
        power_mgmt_update_activity(); // Reset power management timer
        ESP_LOGI(TAG, "Waking from screensaver");
    }
}

// Public functions
esp_err_t oled_ui_init(void)
{
    ESP_LOGI(TAG, "Initializing OLED UI system");
    
    // Initialize OLED display
    esp_err_t ret = bsp_oled_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize OLED: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set default device status
    strcpy(current_status.device_name, "LoRaCue-STAGE");
    current_status.battery_voltage = 3.7f;
    current_status.signal_strength = 85;
    current_status.paired_count = 0;
    
    ui_initialized = true;
    current_state = UI_STATE_BOOT_LOGO;
    
    ESP_LOGI(TAG, "OLED UI initialized successfully");
    return ESP_OK;
}

esp_err_t oled_ui_handle_event(ui_event_t event)
{
    if (!ui_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGD(TAG, "Handling event %d in state %d", event, current_state);
    
    // Update power management activity on any user input
    if (event != UI_EVENT_NONE && event != UI_EVENT_TIMEOUT) {
        power_mgmt_update_activity();
    }
    
    switch (current_state) {
        case UI_STATE_BOOT_LOGO:
            if (event != UI_EVENT_NONE) {
                current_state = UI_STATE_MAIN;
            }
            break;
            
        case UI_STATE_MAIN:
            handle_main_state(event);
            break;
            
        case UI_STATE_MENU:
            handle_menu_state(event);
            break;
            
        case UI_STATE_WIRELESS:
            handle_submenu_state(event, wireless_menu_items, WIRELESS_MENU_COUNT, UI_STATE_MENU);
            break;
            
        case UI_STATE_PAIRING:
            handle_submenu_state(event, pairing_menu_items, PAIRING_MENU_COUNT, UI_STATE_MENU);
            break;
            
        case UI_STATE_PAIRING_PIN:
            // PIN display state - back on any button
            if (event != UI_EVENT_NONE && event != UI_EVENT_TIMEOUT) {
                current_state = UI_STATE_PAIRING;
                menu_selection = 0;
                ESP_LOGI(TAG, "Returning to pairing menu");
            }
            break;
            
        case UI_STATE_SYSTEM:
            handle_submenu_state(event, system_menu_items, SYSTEM_MENU_COUNT, UI_STATE_MENU);
            break;
            
        case UI_STATE_WEB_CONFIG:
            // Web config display state - back on any button
            if (event != UI_EVENT_NONE && event != UI_EVENT_TIMEOUT) {
                current_state = UI_STATE_SYSTEM;
                menu_selection = 0;
                ESP_LOGI(TAG, "Returning to system menu");
            }
            break;
            
        case UI_STATE_SCREENSAVER:
            handle_screensaver_state(event);
            break;
            
        default:
            ESP_LOGW(TAG, "Unhandled state: %d", current_state);
            break;
    }
    
    // Update display based on new state
    switch (current_state) {
        case UI_STATE_BOOT_LOGO:
            draw_boot_logo();
            break;
            
        case UI_STATE_MAIN:
            draw_main_screen();
            break;
            
        case UI_STATE_MENU:
            draw_menu(main_menu_items, MAIN_MENU_COUNT, menu_selection, "Main Menu");
            break;
            
        case UI_STATE_WIRELESS:
            draw_menu(wireless_menu_items, WIRELESS_MENU_COUNT, menu_selection, "Wireless");
            break;
            
        case UI_STATE_PAIRING:
            draw_menu(pairing_menu_items, PAIRING_MENU_COUNT, menu_selection, "Pairing");
            break;
            
        case UI_STATE_PAIRING_PIN:
            draw_pairing_pin("123456"); // Placeholder PIN
            break;
            
        case UI_STATE_SYSTEM:
            draw_menu(system_menu_items, SYSTEM_MENU_COUNT, menu_selection, "System");
            break;
            
        case UI_STATE_WEB_CONFIG:
            draw_web_config("LoRaCue-Config", "192.168.4.1");
            break;
            
        case UI_STATE_SCREENSAVER:
            draw_screensaver();
            break;
            
        default:
            break;
    }
    
    return ESP_OK;
}

esp_err_t oled_ui_update_status(const device_status_t *status)
{
    if (!ui_initialized || !status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&current_status, status, sizeof(device_status_t));
    
    // Refresh display if on main screen
    if (current_state == UI_STATE_MAIN) {
        draw_main_screen();
    }
    
    return ESP_OK;
}

ui_state_t oled_ui_get_state(void)
{
    return current_state;
}

esp_err_t oled_ui_set_state(ui_state_t state)
{
    current_state = state;
    menu_selection = 0;
    return oled_ui_handle_event(UI_EVENT_NONE); // Trigger display update
}

esp_err_t oled_ui_show_boot_logo(void)
{
    return oled_ui_set_state(UI_STATE_BOOT_LOGO);
}

esp_err_t oled_ui_enter_screensaver(void)
{
    return oled_ui_set_state(UI_STATE_SCREENSAVER);
}
