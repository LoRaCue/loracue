#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Button event types for one-button UI
 */
typedef enum {
    BUTTON_EVENT_SHORT = 0, // <500ms
    BUTTON_EVENT_DOUBLE,    // 2 clicks <200ms apart
    BUTTON_EVENT_LONG       // >2s
} button_event_type_t;

/**
 * @brief Command history entry for PC mode
 */
typedef struct {
    uint32_t timestamp_ms __attribute__((unused));
    uint16_t device_id __attribute__((unused));
    char device_name[16] __attribute__((unused));
    char command[8] __attribute__((unused));
    uint8_t keycode __attribute__((unused));
    uint8_t modifiers __attribute__((unused));
} command_history_entry_t;

#ifdef __cplusplus
}
#endif
