/**
 * @file usb_console.c
 * @brief USB CDC console redirection (debug port)
 */

#include "usb_console.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tinyusb_cdc_acm.h"
#include "driver/gpio.h"
#include <stdarg.h>
#include <stdio.h>

static const char *TAG = "USB_CONSOLE";

#ifndef LORACUE_RELEASE

// Store original vprintf for UART output
static vprintf_like_t original_vprintf = NULL;

static int usb_console_vprintf(const char *fmt, va_list args)
{
    // Send to USB CDC port 1 (debug console)
    if (tud_cdc_n_connected(1)) {
        char buf[256];
        int len = vsnprintf(buf, sizeof(buf), fmt, args);
        if (len > 0) {
            tud_cdc_n_write(1, buf, len);
            tud_cdc_n_write_flush(1);
        }
    }
    
    // Also send to UART (dual output)
    if (original_vprintf) {
        return original_vprintf(fmt, args);
    }
    
    return 0;
}

// Handle DTR/RTS signals for bootloader reset on CDC port 1
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    if (itf == 1) {  // Only handle CDC port 1 (debug console)
        // DTR=low, RTS=high = Enter bootloader
        // DTR=high, RTS=low = Reset and run
        if (!dtr && rts) {
            ESP_LOGI(TAG, "Bootloader mode requested via DTR/RTS");
            esp_restart();  // Restart into bootloader
        } else if (dtr && !rts) {
            ESP_LOGI(TAG, "Reset requested via DTR/RTS");
            esp_restart();  // Normal reset
        }
    }
}

esp_err_t usb_console_init(void)
{
    ESP_LOGI(TAG, "Redirecting console to USB CDC port 1 (debug) + UART");
    ESP_LOGI(TAG, "DTR/RTS control enabled for bootloader reset");
    
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
