#include "lvgl.h"
#include "ui_components.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "version.h"
#include "bsp.h"
#include "lora_driver.h"
#include "general_config.h"

static ui_info_screen_t *info_screen = NULL;

void screen_system_info_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    if (!info_screen) {
        info_screen = ui_info_screen_create("SYSTEM INFO");
    }
    
    char line0[64], line1[64], line2[64];
    snprintf(line0, sizeof(line0), "FW: %s", LORACUE_VERSION_FULL);
    snprintf(line1, sizeof(line1), "HW: %s", bsp_get_board_name());
    snprintf(line2, sizeof(line2), "IDF: %s", IDF_VER);
    
    const char *lines[] = {line0, line1, line2};
    ui_info_screen_render(info_screen, parent, "SYSTEM INFO", lines);
}

void screen_device_info_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    if (!info_screen) {
        info_screen = ui_info_screen_create("DEVICE INFO");
    }
    
    general_config_t config;
    general_config_get(&config);
    
    char line0[64], line1[64], line2[64];
    snprintf(line0, sizeof(line0), "Name: %s", config.device_name);
    
    const char *mode_str = (config.device_mode == DEVICE_MODE_PRESENTER) ? "PRESENTER" : "PC";
    snprintf(line1, sizeof(line1), "Mode: %s", mode_str);
    
    uint32_t freq_hz = lora_get_frequency();
    snprintf(line2, sizeof(line2), "LoRa: %u.%u MHz", (unsigned int)(freq_hz / 1000000), (unsigned int)((freq_hz % 1000000) / 100000));
    
    const char *lines[] = {line0, line1, line2};
    ui_info_screen_render(info_screen, parent, "DEVICE INFO", lines);
}

void screen_battery_status_create(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    
    if (!info_screen) {
        info_screen = ui_info_screen_create("BATTERY");
    }
    
    float voltage = bsp_read_battery();
    uint8_t percentage;
    if (voltage >= 4.2f) {
        percentage = 100;
    } else if (voltage <= 3.0f) {
        percentage = 0;
    } else {
        percentage = (uint8_t)((voltage - 3.0f) / 1.2f * 100);
    }
    
    const char *health = voltage < 3.2f ? "Critical" : voltage < 3.5f ? "Low" : "Good";
    
    char line0[64], line1[64], line2[64];
    snprintf(line0, sizeof(line0), "Level: %u%%", percentage);
    snprintf(line1, sizeof(line1), "Voltage: %.1fV", voltage);
    snprintf(line2, sizeof(line2), "Health: %s", health);
    
    const char *lines[] = {line0, line1, line2};
    ui_info_screen_render(info_screen, parent, "BATTERY", lines);
}
