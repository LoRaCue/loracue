#include "usb_cdc.h"
#include "commands.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb.h"
#include <string.h>

static const char *TAG = "USB_CDC";
static char rx_buffer[512];
static size_t rx_len = 0;

static void send_response(const char *response)
{
    tud_cdc_write_str(response);
    tud_cdc_write_str("\n");
    tud_cdc_write_flush();
}

static void process_command(const char *command_line)
{
    ESP_LOGI(TAG, "Processing command: %s", command_line);
    commands_execute(command_line, send_response);
}

void usb_cdc_process_commands(void)
{
    if (!tud_cdc_connected()) {
        return;
    }

    while (tud_cdc_available()) {
        char c = tud_cdc_read_char();

        if (c == '\n' || c == '\r') {
            if (rx_len > 0) {
                rx_buffer[rx_len] = '\0';
                process_command(rx_buffer);
                rx_len = 0;
            }
        } else if (rx_len < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_len++] = c;
        } else {
            // Buffer overflow, reset
            rx_len = 0;
            send_response("ERROR Command too long");
        }
    }
}

esp_err_t usb_cdc_init(void)
{
    ESP_LOGI(TAG, "USB CDC protocol initialized");
    return ESP_OK;
}
