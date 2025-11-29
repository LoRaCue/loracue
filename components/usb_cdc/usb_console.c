/**
 * @file usb_console.c
 * @brief USB CDC console redirection (debug port)
 */

#include "usb_console.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"  // Renamed in esp_tinyusb 1.7.6+
#include "driver/gpio.h"
#include <stdarg.h>
#include <stdio.h>

static const char *TAG = "USB_CONSOLE";

#ifndef LORACUE_RELEASE

// Store original vprintf for UART output
static vprintf_like_t original_vprintf = NULL;

static int usb_console_vprintf(const char *fmt, va_list args)
{
    // Send to USB CDC port 0 (same port as commands)
    if (tud_cdc_connected()) {
        char buf[256];
        int len = vsnprintf(buf, sizeof(buf), fmt, args);
        if (len > 0) {
            tud_cdc_write(buf, len);
            tud_cdc_write_flush();
        }
    }
    
    return 0;
}

esp_err_t usb_console_init(void)
{
    ESP_LOGI(TAG, "Redirecting console to USB CDC port 0");
    
    // Save original vprintf (UART output)
    original_vprintf = esp_log_set_vprintf(usb_console_vprintf);
    
    return ESP_OK;
}

#else

esp_err_t usb_console_init(void)
{
    // No-op in release builds
    return ESP_OK;
}

#endif
