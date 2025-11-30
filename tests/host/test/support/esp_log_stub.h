#ifndef ESP_LOG_STUB_H
#define ESP_LOG_STUB_H

#include <stdio.h>

#define ESP_LOGI(tag, format, ...) printf("[INFO][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) printf("[ERROR][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("[WARN][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) printf("[DEBUG][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOG_BUFFER_HEX(tag, buf, len) do { \
    printf("[HEX][%s] ", tag); \
    for(int i=0; i<(int)(len); i++) printf("%02X ", ((uint8_t*)(buf))[i]); \
    printf("\n"); \
} while(0)

#endif
