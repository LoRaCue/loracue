#ifndef MOCK_ESP_SYSTEM_H
#define MOCK_ESP_SYSTEM_H

#include <stdint.h>
#include <stddef.h>

void esp_fill_random(void *buf, size_t len);
uint32_t esp_random(void);
uint64_t esp_timer_get_time(void);

#endif
