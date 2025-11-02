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
    uint32_t timestamp_ms;
    uint16_t device_id;
    char device_name[16];
    char command[8];
    uint8_t keycode;
    uint8_t modifiers;
} command_history_entry_t;

#ifdef __cplusplus
}
#endif
