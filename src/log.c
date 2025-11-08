#include "log.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_CDC_ITF 1
#define LOG_BUFFER_SIZE 256

static log_level_t current_level = LOG_INFO;
static char log_buffer[LOG_BUFFER_SIZE];

static const char *level_strings[] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR"
};

void log_init(void) {
    current_level = LOG_INFO;
}

void log_set_level(log_level_t level) {
    current_level = level;
}

void log_printf(log_level_t level, const char *fmt, ...) {
    if (level < current_level) return;
    if (!tud_cdc_n_connected(LOG_CDC_ITF)) return;
    
    uint32_t ms = to_ms_since_boot(get_absolute_time());
    int len = snprintf(log_buffer, LOG_BUFFER_SIZE, "[%6lu.%03lu] %s: ",
                      ms / 1000, ms % 1000, level_strings[level]);
    
    va_list args;
    va_start(args, fmt);
    len += vsnprintf(log_buffer + len, LOG_BUFFER_SIZE - len, fmt, args);
    va_end(args);
    
    if (len >= LOG_BUFFER_SIZE - 2) {
        len = LOG_BUFFER_SIZE - 2;
    }
    log_buffer[len++] = '\n';
    log_buffer[len] = '\0';
    
    tud_cdc_n_write(LOG_CDC_ITF, log_buffer, len);
    tud_cdc_n_write_flush(LOG_CDC_ITF);
}
