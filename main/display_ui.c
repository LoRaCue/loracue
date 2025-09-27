#include "display_ui.h"
#include "esp_log.h"

static const char *TAG = "DISPLAY_UI";

// Simple LoRaCue logo as ASCII art for now
static const char* LORACUE_LOGO[] = {
    "    LoRaCue    ",
    "  Presentation ",
    "    Clicker    ",
    "               ",
    "   v1.0.0-dev  "
};

esp_err_t display_ui_init(void)
{
    ESP_LOGI(TAG, "Initializing display UI");
    
    // TODO: Initialize SH1106 OLED driver
    // For now, just log that we're ready
    ESP_LOGI(TAG, "Display UI ready (placeholder)");
    
    return ESP_OK;
}

void display_show_boot_logo(void)
{
    ESP_LOGI(TAG, "Showing boot logo");
    
    // TODO: Display actual logo on OLED
    // For now, print to console
    printf("\n");
    for (int i = 0; i < 5; i++) {
        printf("  %s\n", LORACUE_LOGO[i]);
    }
    printf("\n");
}

void display_show_main_screen(const char* device_name, float battery_voltage, int signal_strength)
{
    ESP_LOGI(TAG, "Updating main screen: %s, %.2fV, %d%%", device_name, battery_voltage, signal_strength);
    
    // TODO: Display on actual OLED
    // For now, print to console
    printf("\n=== LoRaCue Main Screen ===\n");
    printf("Device: %s\n", device_name);
    printf("Battery: %.2fV\n", battery_voltage);
    printf("Signal: %d%%\n", signal_strength);
    printf("Status: Ready\n");
    printf("==========================\n\n");
}

void display_clear(void)
{
    ESP_LOGI(TAG, "Clearing display");
    // TODO: Clear actual OLED display
}

void display_set_brightness(uint8_t brightness)
{
    ESP_LOGI(TAG, "Setting display brightness to %d", brightness);
    // TODO: Set actual OLED brightness
}
