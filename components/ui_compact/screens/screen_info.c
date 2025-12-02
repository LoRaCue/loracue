#include "input_manager.h"
#include "ui_strings.h"
#include "bsp.h"
#include "input_manager.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "screens.h"
#include "ui_components.h"
#include "ui_navigator.h"
#include "version.h"

static ui_text_viewer_t *system_viewer  = NULL;
static ui_text_viewer_t *device_viewer  = NULL;
static ui_text_viewer_t *battery_viewer = NULL;

static char device_lines_buffer[15][64];
static const char *device_lines[15];

static char system_lines_buffer[3][64];
static const char *system_lines[3];

static char battery_lines_buffer[3][64];
static const char *battery_lines[3];

void screen_system_info_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    if (!system_viewer) {
        snprintf(system_lines_buffer[0], 64, "FW: %s", LORACUE_VERSION_FULL);
        snprintf(system_lines_buffer[1], 64, "HW: %s", bsp_get_board_name());
        snprintf(system_lines_buffer[2], 64, "IDF: %s", IDF_VER);

        system_lines[0] = system_lines_buffer[0];
        system_lines[1] = system_lines_buffer[1];
        system_lines[2] = system_lines_buffer[2];

        system_viewer = ui_text_viewer_create(system_lines, 3);
    }

    ui_text_viewer_render(system_viewer, parent, "SYSTEM INFO");
}

void screen_device_info_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    if (!device_viewer) {
        // Prepare all device info lines
        uint8_t idx = 0;

        // Hardware info first
        // Model
        snprintf(device_lines_buffer[idx], 64, "Model: %s", bsp_get_model_name());
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
        snprintf(device_lines_buffer[idx], 64, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
                 mac[4], mac[5]);
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

        // Battery info
        float voltage      = bsp_read_battery();
        uint8_t percentage = bsp_battery_voltage_to_percentage(voltage);
        snprintf(device_lines_buffer[idx], 64, "Battery: %u%% (%.1fV)", percentage, voltage);
        device_lines[idx] = device_lines_buffer[idx];
        idx++;

        device_viewer = ui_text_viewer_create(device_lines, idx);
    }

    ui_text_viewer_render(device_viewer, parent, "DEVICE INFO");
}

static void screen_device_info_reset(void)
{
    if (device_viewer) {
        ui_text_viewer_destroy(device_viewer);
        device_viewer = NULL;
    }
    if (system_viewer) {
        ui_text_viewer_destroy(system_viewer);
        system_viewer = NULL;
    }
    if (battery_viewer) {
        ui_text_viewer_destroy(battery_viewer);
        battery_viewer = NULL;
    }
}

void screen_battery_status_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    if (!battery_viewer) {
        float voltage      = bsp_read_battery();
        uint8_t percentage = bsp_battery_voltage_to_percentage(voltage);

        const char *health = voltage < 3.2f ? "Critical" : voltage < 3.5f ? "Low" : "Good";

        snprintf(battery_lines_buffer[0], 64, "Level: %u%%", percentage);
        snprintf(battery_lines_buffer[1], 64, "Voltage: %.1fV", voltage);
        snprintf(battery_lines_buffer[2], 64, "Health: %s", health);

        battery_lines[0] = battery_lines_buffer[0];
        battery_lines[1] = battery_lines_buffer[1];
        battery_lines[2] = battery_lines_buffer[2];

        battery_viewer = ui_text_viewer_create(battery_lines, 3);
    }

    ui_text_viewer_render(battery_viewer, parent, "BATTERY");
}

// --- Input Handlers & Interfaces ---

static void handle_system_info_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_SHORT) {
        if (system_viewer) {
            ui_text_viewer_scroll(system_viewer);
            lv_obj_t *screen = lv_scr_act();
            lv_obj_clean(screen);
            screen_system_info_create(screen);
        }
    } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
            ui_navigator_switch_to(UI_SCREEN_MENU);
            break;
        case INPUT_EVENT_ENCODER_CW:
        case INPUT_EVENT_NEXT_SHORT:
            if (system_viewer) {
                ui_text_viewer_scroll(system_viewer);
                lv_obj_t *screen = lv_scr_act();
                lv_obj_clean(screen);
                screen_system_info_create(screen);
            }
            break;
        case INPUT_EVENT_ENCODER_CCW:
            if (system_viewer) {
                ui_text_viewer_scroll_back(system_viewer);
                lv_obj_t *screen = lv_scr_act();
                lv_obj_clean(screen);
                screen_system_info_create(screen);
            }
            break;
        default:
            break;
    }
#endif
}

static void screen_system_info_reset(void)
{
    if (system_viewer) {
        ui_text_viewer_destroy(system_viewer);
        system_viewer = NULL;
    }
}

static ui_screen_t system_info_screen = {
    .type               = UI_SCREEN_SYSTEM_INFO,
    .create             = screen_system_info_create,
    .destroy            = screen_system_info_reset,
    .handle_input_event = handle_system_info_input_event,
};

ui_screen_t *screen_system_info_get_interface(void)
{
    return &system_info_screen;
}

static void handle_device_info_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_SHORT) {
        if (device_viewer) {
            ui_text_viewer_scroll(device_viewer);
            lv_obj_t *screen = lv_scr_act();
            lv_obj_clean(screen);
            screen_device_info_create(screen);
        }
    } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
            ui_navigator_switch_to(UI_SCREEN_MENU);
            break;
        case INPUT_EVENT_ENCODER_CW:
        case INPUT_EVENT_NEXT_SHORT:
            if (device_viewer) {
                ui_text_viewer_scroll(device_viewer);
                lv_obj_t *screen = lv_scr_act();
                lv_obj_clean(screen);
                screen_device_info_create(screen);
            }
            break;
        case INPUT_EVENT_ENCODER_CCW:
            if (device_viewer) {
                ui_text_viewer_scroll_back(device_viewer);
                lv_obj_t *screen = lv_scr_act();
                lv_obj_clean(screen);
                screen_device_info_create(screen);
            }
            break;
        default:
            break;
    }
#endif
}

static ui_screen_t device_info_screen = {
    .type               = UI_SCREEN_DEVICE_INFO,
    .create             = screen_device_info_create,
    .destroy            = screen_device_info_reset,
    .handle_input_event = handle_device_info_input_event,
};

ui_screen_t *screen_device_info_get_interface(void)
{
    return &device_info_screen;
}

static void handle_battery_status_input_event(input_event_t event)
{
#if CONFIG_LORACUE_MODEL_ALPHA
    if (event == INPUT_EVENT_NEXT_SHORT) {
        if (battery_viewer) {
            ui_text_viewer_scroll(battery_viewer);
            lv_obj_t *screen = lv_scr_act();
            lv_obj_clean(screen);
            screen_battery_status_create(screen);
        }
    } else if (event == INPUT_EVENT_NEXT_DOUBLE) {
        ui_navigator_switch_to(UI_SCREEN_MENU);
    }
#elif CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    switch (event) {
        case INPUT_EVENT_PREV_SHORT:
            ui_navigator_switch_to(UI_SCREEN_MENU);
            break;
        case INPUT_EVENT_ENCODER_CW:
        case INPUT_EVENT_NEXT_SHORT:
            if (battery_viewer) {
                ui_text_viewer_scroll(battery_viewer);
                lv_obj_t *screen = lv_scr_act();
                lv_obj_clean(screen);
                screen_battery_status_create(screen);
            }
            break;
        case INPUT_EVENT_ENCODER_CCW:
            if (battery_viewer) {
                ui_text_viewer_scroll_back(battery_viewer);
                lv_obj_t *screen = lv_scr_act();
                lv_obj_clean(screen);
                screen_battery_status_create(screen);
            }
            break;
        default:
            break;
    }
#endif
}

static void screen_battery_status_reset(void)
{
    if (battery_viewer) {
        ui_text_viewer_destroy(battery_viewer);
        battery_viewer = NULL;
    }
}

static ui_screen_t battery_status_screen = {
    .type               = UI_SCREEN_BATTERY,
    .create             = screen_battery_status_create,
    .destroy            = screen_battery_status_reset,
    .handle_input_event = handle_battery_status_input_event,
};

ui_screen_t *screen_battery_status_get_interface(void)
{
    return &battery_status_screen;
}
