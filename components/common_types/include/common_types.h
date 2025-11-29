#pragma once

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create mutex or fail with error
 * 
 * Usage: CREATE_MUTEX_OR_FAIL(state_mutex);
 */
#define CREATE_MUTEX_OR_FAIL(mutex_var)                      \
    do {                                                     \
        (mutex_var) = xSemaphoreCreateMutex();              \
        if (!(mutex_var)) {                                 \
            ESP_LOGE(TAG, "Failed to create mutex: " #mutex_var); \
            return ESP_ERR_NO_MEM;                          \
        }                                                    \
    } while (0)

/**
 * @brief Check if component is initialized
 * 
 * Usage: CHECK_INITIALIZED(protocol_initialized, "LoRa protocol");
 */
#define CHECK_INITIALIZED(flag, component_name)              \
    do {                                                     \
        if (!(flag)) {                                      \
            ESP_LOGE(TAG, component_name " not initialized"); \
            return ESP_ERR_INVALID_STATE;                   \
        }                                                    \
    } while (0)

/**
 * @brief Get current time in milliseconds
 * 
 * @return Current time in milliseconds
 */
static inline uint32_t get_time_ms(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

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
