/**
 * @file uart_commands.c
 * @brief UART command interface implementation
 */

#include "uart_commands.h"

#ifdef CONFIG_LORACUE_UART_COMMANDS_ENABLED

#include "bsp.h"
#include "commands.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "UART_CMD";

// UART configuration from Kconfig
#ifdef CONFIG_LORACUE_UART_COMMANDS_PORT_NUM
#define UART_PORT_NUM CONFIG_LORACUE_UART_COMMANDS_PORT_NUM
#else
#define UART_PORT_NUM 1 // Default to UART1 if not configured
#endif

#define UART_BAUD_RATE 460800
#define UART_RX_BUF_SIZE 8192   // Large RX buffer for high-speed bursts
#define UART_TX_BUF_SIZE 8192   // Large TX buffer for JSON responses
#define CMD_MAX_LENGTH 2048     // Max single command length
#define RX_FLOW_CTRL_THRESH 100 // De-assert RTS when hardware FIFO reaches 100 bytes (max 127)
#define UART_READ_TIMEOUT_MS 20
#define CMD_QUEUE_TIMEOUT_MS 100
#define UART_RX_TASK_STACK_SIZE 3072
#define UART_RX_TASK_PRIORITY 10
#define CMD_PROC_TASK_STACK_SIZE 4096
#define CMD_PROC_TASK_PRIORITY 5

static TaskHandle_t uart_rx_task_handle       = NULL;
static TaskHandle_t cmd_processor_task_handle = NULL;
static QueueHandle_t cmd_queue                = NULL;
static bool uart_running                      = false;
static SemaphoreHandle_t uart_tx_mutex        = NULL;

static void send_response(const char *response)
{
    if (response && uart_tx_mutex) {
        xSemaphoreTake(uart_tx_mutex, portMAX_DELAY);

        uart_write_bytes(uart_num, response, strlen(response));
        uart_write_bytes(uart_num, "\r\n", 2);

        xSemaphoreGive(uart_tx_mutex);
    }
}

// High-priority RX task: only reads UART and queues commands
static void uart_rx_task(void *pvParameters)
{
    uint8_t data[256]; // Small buffer for fast processing
    char *line_buffer = heap_caps_malloc(CMD_MAX_LENGTH, MALLOC_CAP_8BIT);
    if (!line_buffer) {
        ESP_LOGE(TAG, "Failed to allocate RX line buffer");
        vTaskDelete(NULL);
        return;
    }
    size_t line_pos = 0;

    ESP_LOGI(TAG, "UART RX task started (high priority)");

    while (uart_running) {
        int len = uart_read_bytes(uart_num, data, sizeof(data), pdMS_TO_TICKS(UART_READ_TIMEOUT_MS));

        for (int i = 0; i < len; i++) {
            char c = (char)data[i];

            if (c == '\n' || c == '\r') {
                if (line_pos > 0) {
                    line_buffer[line_pos] = '\0';

                    // Allocate command string for queue
                    char *cmd = heap_caps_malloc(line_pos + 1, MALLOC_CAP_8BIT);
                    if (cmd) {
                        memcpy(cmd, line_buffer, line_pos + 1);
                        if (xQueueSend(cmd_queue, &cmd, 0) != pdTRUE) {
                            ESP_LOGW(TAG, "Command queue full, dropping: %s", cmd);
                            free(cmd);
                        }
                    }
                    line_pos = 0;
                }
            } else if (c == '\b' || c == 127) {
                if (line_pos > 0)
                    line_pos--;
            } else if (line_pos < CMD_MAX_LENGTH - 1 && c >= 32 && c < 127) {
                line_buffer[line_pos++] = c;
            }
        }
    }

    free(line_buffer);
    ESP_LOGI(TAG, "UART RX task stopped");
    vTaskDelete(NULL);
}

// Lower-priority command processor: executes commands from queue
static void cmd_processor_task(void *pvParameters)
{
    char *cmd = NULL;

    ESP_LOGI(TAG, "Command processor task started");

    while (uart_running) {
        if (xQueueReceive(cmd_queue, &cmd, pdMS_TO_TICKS(CMD_QUEUE_TIMEOUT_MS)) == pdTRUE) {
            ESP_LOGI(TAG, "Processing: %s", cmd);
            commands_execute(cmd, send_response);
            free(cmd);
            cmd = NULL;
        }
    }

    ESP_LOGI(TAG, "Command processor task stopped");
    vTaskDelete(NULL);
}

esp_err_t uart_commands_init(void)
{
    // Get UART pins from BSP
    int tx_pin, rx_pin;
    esp_err_t ret = bsp_get_uart_pins(UART_PORT_NUM, &tx_pin, &rx_pin);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get UART%d pins from BSP", UART_PORT_NUM);
        return ret;
    }

    ESP_LOGI(TAG, "Initializing UART%d command interface (TX=%d, RX=%d, %d baud)", UART_PORT_NUM, tx_pin, rx_pin,
             UART_BAUD_RATE);

    // Create TX mutex for thread-safe HAL writes
    uart_tx_mutex = xSemaphoreCreateMutex();
    if (!uart_tx_mutex) {
        ESP_LOGE(TAG, "Failed to create UART TX mutex");
        return ESP_FAIL;
    }

    // Delete existing driver if present
    uart_driver_delete(UART_PORT_NUM);

    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate  = UART_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, uart_tx_pin, uart_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Install with large buffers and event queue
    QueueHandle_t uart_queue;
    esp_err_t ret = uart_driver_install(uart_num, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 20, &uart_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "UART%d driver installed: RX=%d, TX=%d, no flow control", uart_num, UART_RX_BUF_SIZE,
             UART_TX_BUF_SIZE);

    ESP_LOGI(TAG, "UART%d configured: %d baud, TX=%d, RX=%d", uart_num, UART_BAUD_RATE, uart_tx_pin, uart_rx_pin);

    return ESP_OK;
}

esp_err_t uart_commands_start(void)
{
    if (uart_running) {
        ESP_LOGW(TAG, "UART command tasks already running");
        return ESP_OK;
    }

    // Create command queue
    cmd_queue = xQueueCreate(10, sizeof(char *));
    if (!cmd_queue) {
        ESP_LOGE(TAG, "Failed to create command queue");
        return ESP_FAIL;
    }

    uart_running = true;

    // High-priority RX task (priority 10) - must always be responsive
    BaseType_t ret = xTaskCreate(uart_rx_task, "uart_rx", UART_RX_TASK_STACK_SIZE, NULL, UART_RX_TASK_PRIORITY,
                                 &uart_rx_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART RX task");
        uart_running = false;
        vQueueDelete(cmd_queue);
        return ESP_FAIL;
    }

    // Lower-priority command processor (priority 5) - can be preempted
    BaseType_t ret = xTaskCreate(cmd_processor_task, "cmd_proc", CMD_PROC_TASK_STACK_SIZE, NULL, CMD_PROC_TASK_PRIORITY,
                                 &cmd_processor_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create command processor task");
        uart_running = false;
        vTaskDelete(uart_rx_task_handle);
        vQueueDelete(cmd_queue);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "UART tasks started: RX (prio 10), CMD processor (prio 5)");
    return ESP_OK;
}

esp_err_t uart_commands_stop(void)
{
    if (!uart_running) {
        return ESP_OK;
    }

    uart_running = false;

    // Clean up queue (free any pending commands)
    if (cmd_queue) {
        char *cmd;
        while (xQueueReceive(cmd_queue, &cmd, 0) == pdTRUE) {
            free(cmd);
        }
        vQueueDelete(cmd_queue);
        cmd_queue = NULL;
    }

    uart_rx_task_handle       = NULL;
    cmd_processor_task_handle = NULL;

    ESP_LOGI(TAG, "UART command tasks stopped");
    return ESP_OK;
}

#else // CONFIG_LORACUE_UART_COMMANDS_ENABLED not defined

// Stub implementations when UART commands are disabled
esp_err_t uart_commands_init(void)
{
    return ESP_OK;
}

esp_err_t uart_commands_start(void)
{
    return ESP_OK;
}

esp_err_t uart_commands_stop(void)
{
    return ESP_OK;
}

#endif // CONFIG_LORACUE_UART_COMMANDS_ENABLED
