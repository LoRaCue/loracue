#include "lvgl.h"
#include "ui_components.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_flash.h"
#include "esp_timer.h"
#include "version.h"
#include "bsp.h"
#include "lora_driver.h"
#include "general_config.h"

static ui_info_screen_t *info_screen = NULL;
static ui_text_viewer_t *device_viewer = NULL;
static char device_lines_buffer[15][64];
static const char *device_lines[15];

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
    
    if (!device_viewer) {
        // Prepare all device info lines
        uint8_t idx = 0;
        
        // Hardware info first
        // Model
        snprintf(device_lines_buffer[idx], 64, "Model: LC-Alpha");
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Board ID
        snprintf(device_lines_buffer[idx], 64, "Board: %s", bsp_get_board_id());
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Chip model
        snprintf(device_lines_buffer[idx], 64, "Chip: %s", CONFIG_IDF_TARGET);
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Chip revision & cores
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        snprintf(device_lines_buffer[idx], 64, "Rev: %d Cores: %d", chip_info.revision, chip_info.cores);
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Flash size
        uint32_t flash_size = 0;
        esp_flash_get_size(NULL, &flash_size);
        snprintf(device_lines_buffer[idx], 64, "Flash: %u MB", (unsigned int)(flash_size / (1024 * 1024)));
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // MAC address
        uint8_t mac[6];
        esp_efuse_mac_get_default(mac);
        snprintf(device_lines_buffer[idx], 64, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", 
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Firmware info
        // Version
        snprintf(device_lines_buffer[idx], 64, "Ver: %s", LORACUE_VERSION_STRING);
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Commit
        snprintf(device_lines_buffer[idx], 64, "Commit: %s", LORACUE_BUILD_COMMIT_SHORT);
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Branch
        snprintf(device_lines_buffer[idx], 64, "Branch: %s", LORACUE_BUILD_BRANCH);
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Build date
        snprintf(device_lines_buffer[idx], 64, "Built: %s", LORACUE_BUILD_DATE);
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Runtime info
        // Uptime
        uint32_t uptime_sec = esp_timer_get_time() / 1000000;
        snprintf(device_lines_buffer[idx], 64, "Uptime: %u sec", (unsigned int)uptime_sec);
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Free heap
        snprintf(device_lines_buffer[idx], 64, "Heap: %u KB", (unsigned int)(esp_get_free_heap_size() / 1024));
        device_lines[idx] = device_lines_buffer[idx];
        idx++;
        
        // Partition
        const esp_partition_t *running = esp_ota_get_running_partition();
        if (running) {
            snprintf(device_lines_buffer[idx], 64, "Part: %s", running->label);
            device_lines[idx] = device_lines_buffer[idx];
            idx++;
        }
        
        device_viewer = ui_text_viewer_create(device_lines, idx);
    }
    
    ui_text_viewer_render(device_viewer, parent, "DEVICE INFO");
}

void screen_device_info_button_press(void) {
    if (device_viewer) {
        ui_text_viewer_scroll(device_viewer);
    }
}

void screen_device_info_reset(void) {
    if (device_viewer) {
        ui_text_viewer_destroy(device_viewer);
        device_viewer = NULL;
    }
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
