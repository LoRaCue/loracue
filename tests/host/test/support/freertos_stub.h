#ifndef FREERTOS_STUB_H
#define FREERTOS_STUB_H

#include <stdint.h>

typedef void* TaskHandle_t;
typedef void* QueueHandle_t;

#define portTICK_PERIOD_MS 1

static inline void vTaskDelay(uint32_t ms) { (void)ms; }
static inline void vTaskDelete(void* task) { (void)task; }

#endif
