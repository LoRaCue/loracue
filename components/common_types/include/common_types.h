#pragma once

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

#ifdef __cplusplus
}
#endif
