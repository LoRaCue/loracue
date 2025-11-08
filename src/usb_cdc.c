#include "usb_cdc.h"
#include "tusb.h"
#include "pico/bootrom.h"
#include <string.h>

static uint8_t cmd_buffer[64];
static int cmd_len = 0;
static bool last_dtr_state = false;

void usb_cdc_init(void) {
    tusb_init();
}

void usb_cdc_task(void) {
    tud_task();
}

bool usb_cdc_connected(void) {
    return tud_cdc_n_connected(CDC_ITF_DATA);
}

int usb_cdc_read(uint8_t *buffer, int len) {
    if (!tud_cdc_n_available(CDC_ITF_DATA)) {
        return 0;
    }
    return tud_cdc_n_read(CDC_ITF_DATA, buffer, len);
}

int usb_cdc_write(const uint8_t *buffer, int len) {
    if (!tud_cdc_n_connected(CDC_ITF_DATA)) {
        return 0;
    }
    return tud_cdc_n_write(CDC_ITF_DATA, buffer, len);
}

void usb_cdc_check_bootloader_cmd(void) {
    // Check DTR on debug interface
    if (tud_cdc_n_connected(CDC_ITF_DEBUG)) {
        bool dtr = tud_cdc_n_get_line_state(CDC_ITF_DEBUG) & 0x01;
        
        // Detect DTR falling edge (1->0 transition)
        if (last_dtr_state && !dtr) {
            reset_usb_boot(0, 0);
        }
        last_dtr_state = dtr;
    }
    
    // Also check for text command
    if (!tud_cdc_n_available(CDC_ITF_DEBUG)) {
        return;
    }
    
    while (tud_cdc_n_available(CDC_ITF_DEBUG) && cmd_len < sizeof(cmd_buffer) - 1) {
        uint8_t c;
        tud_cdc_n_read(CDC_ITF_DEBUG, &c, 1);
        
        if (c == '\n' || c == '\r') {
            if (cmd_len > 0) {
                cmd_buffer[cmd_len] = '\0';
                
                if (strcmp((char*)cmd_buffer, "BOOTLOADER") == 0 ||
                    strcmp((char*)cmd_buffer, "bootloader") == 0 ||
                    strcmp((char*)cmd_buffer, "reboot") == 0) {
                    tud_cdc_n_write_str(CDC_ITF_DEBUG, "Entering bootloader mode...\n");
                    tud_cdc_n_write_flush(CDC_ITF_DEBUG);
                    sleep_ms(100);
                    reset_usb_boot(0, 0);
                }
                
                cmd_len = 0;
            }
        } else {
            cmd_buffer[cmd_len++] = c;
        }
    }
    
    if (cmd_len >= sizeof(cmd_buffer) - 1) {
        cmd_len = 0;
    }
}
