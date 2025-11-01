#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include "common_types.h"
#include "general_config.h"  // For device_mode_t
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(SYSTEM_EVENTS);

typedef enum {
    SYSTEM_EVENT_BATTERY_CHANGED,
    SYSTEM_EVENT_USB_CHANGED,
    SYSTEM_EVENT_LORA_STATE_CHANGED,
    SYSTEM_EVENT_LORA_COMMAND_RECEIVED,
    SYSTEM_EVENT_BUTTON_PRESSED,
    SYSTEM_EVENT_OTA_PROGRESS,
    SYSTEM_EVENT_MODE_CHANGED,
} system_event_id_t;

typedef struct {
    uint8_t level;
    bool charging;
} system_event_battery_t;

typedef struct {
    bool connected;
} system_event_usb_t;

typedef struct {
    bool connected;
    int8_t rssi;
} system_event_lora_t;

typedef struct {
    char command[16];
    int8_t rssi;
} system_event_lora_cmd_t;

typedef struct {
    button_event_type_t type;
} system_event_button_t;

typedef struct {
    uint8_t percent;
    char status[32];
} system_event_ota_t;

typedef struct {
    device_mode_t mode;
} system_event_mode_t;

/**
 * @brief Initialize system event loop
 */
esp_err_t system_events_init(void);

/**
 * @brief Get the system event loop handle
 */
esp_event_loop_handle_t system_events_get_loop(void);

/**
 * @brief Post battery event
 */
esp_err_t system_events_post_battery(uint8_t level, bool charging);

/**
 * @brief Post USB event
 */
esp_err_t system_events_post_usb(bool connected);

/**
 * @brief Post LoRa state event
 */
esp_err_t system_events_post_lora_state(bool connected, int8_t rssi);

/**
 * @brief Post LoRa command event
 */
esp_err_t system_events_post_lora_command(const char *command, int8_t rssi);

/**
 * @brief Post button event
 */
esp_err_t system_events_post_button(button_event_type_t type);

/**
 * @brief Post OTA progress event
 */
esp_err_t system_events_post_ota_progress(uint8_t percent, const char *status);

/**
 * @brief Post mode change event
 */
esp_err_t system_events_post_mode_changed(device_mode_t mode);

#ifdef __cplusplus
}
#endif
