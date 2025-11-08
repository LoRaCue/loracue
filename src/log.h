#ifndef LOG_H
#define LOG_H

#include <stdint.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

void log_init(void);
void log_set_level(log_level_t level);
void log_printf(log_level_t level, const char *fmt, ...);

#define LOG_DEBUG(...) log_printf(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log_printf(LOG_INFO, __VA_ARGS__)
#define LOG_WARN(...) log_printf(LOG_WARN, __VA_ARGS__)
#define LOG_ERROR(...) log_printf(LOG_ERROR, __VA_ARGS__)

#endif
