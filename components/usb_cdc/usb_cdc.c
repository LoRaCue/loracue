#include "usb_cdc.h"
#include "commands.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb.h"
#include "class/cdc/cdc_device.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "USB_CDC";

#define CDC_RX_QUEUE_SIZE 10
#define CDC_TASK_STACK_SIZE 4096
#define CDC_TASK_PRIORITY 5
#define CMD_MAX_LENGTH 2048

typedef struct {
    char data[CMD_MAX_LENGTH];
    size_t len;
} cdc_rx_msg_t;

static QueueHandle_t cdc_rx_queue = NULL;
static TaskHandle_t cdc_task_handle = NULL;
static char rx_buffer[CMD_MAX_LENGTH];
static size_t rx_len = 0;

static void send_response(const char *response)
{
    if (response && tud_cdc_connected()) {
        tud_cdc_write_str(response);
        tud_cdc_write_str("\n");
        tud_cdc_write_flush();
    }
}

static void process_command(const char *command_line)
{
    ESP_LOGD(TAG, "Processing command: %s", command_line);
    commands_execute(command_line, send_response);
}

static void cdc_task(void *arg)
{
    ESP_LOGI(TAG, "CDC task started");
    cdc_rx_msg_t msg;
    
    while (1) {
        if (xQueueReceive(cdc_rx_queue, &msg, portMAX_DELAY) == pdTRUE) {
            msg.data[msg.len] = '\0';
            process_command(msg.data);
        }
    }
}

// Called from TinyUSB ISR context - must be non-blocking
void tud_cdc_rx_cb(uint8_t itf)
{
    if (!tud_cdc_connected()) {
        return;
    }

    while (tud_cdc_available()) {
        char c = tud_cdc_read_char();

        if (c == '\n' || c == '\r') {
            if (rx_len > 0) {
                cdc_rx_msg_t msg;
                msg.len = rx_len;
                memcpy(msg.data, rx_buffer, rx_len);
                
                if (xQueueSend(cdc_rx_queue, &msg, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "CDC RX queue full, dropping command");
                }
                rx_len = 0;
            }
        } else if (rx_len < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_len++] = c;
        } else {
            // Buffer overflow, reset
            ESP_LOGW(TAG, "Command too long, dropping");
            rx_len = 0;
        }
    }
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    ESP_LOGD(TAG, "CDC line state changed: itf=%d, dtr=%d, rts=%d", itf, dtr, rts);
}

esp_err_t usb_cdc_init(void)
{
    ESP_LOGI(TAG, "Initializing USB CDC command interface");
    
    // Create RX queue
    cdc_rx_queue = xQueueCreate(CDC_RX_QUEUE_SIZE, sizeof(cdc_rx_msg_t));
    if (!cdc_rx_queue) {
        ESP_LOGE(TAG, "Failed to create CDC RX queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Create CDC task
    BaseType_t ret = xTaskCreate(cdc_task, "cdc", CDC_TASK_STACK_SIZE, NULL, 
                                  CDC_TASK_PRIORITY, &cdc_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create CDC task");
        vQueueDelete(cdc_rx_queue);
        cdc_rx_queue = NULL;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "USB CDC initialized (queue=%d, stack=%d, priority=%d)",
             CDC_RX_QUEUE_SIZE, CDC_TASK_STACK_SIZE, CDC_TASK_PRIORITY);
    return ESP_OK;
}
