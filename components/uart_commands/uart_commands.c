/**
 * @file uart_commands.c
 * @brief UART command interface implementation
 */

#include "uart_commands.h"

#ifdef CONFIG_UART_COMMANDS_ENABLED

#include "commands.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "UART_CMD";

// UART configuration
// UART0 = Command parser (USB/Serial)
// UART1 = Console logging (except when console is on UART0)
#ifdef CONFIG_UART_COMMANDS_PORT_NUM
#define UART_NUM ((uart_port_t)CONFIG_UART_COMMANDS_PORT_NUM)
#else
#define UART_NUM UART_NUM_0
#endif

// Pin configuration
#if CONFIG_UART_COMMANDS_PORT_NUM == 0
// UART0 - default for commands
#define UART_TX_PIN 43
#define UART_RX_PIN 44
#else
// UART1
#define UART_TX_PIN 2
#define UART_RX_PIN 3
#endif

#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 16384
#define CMD_MAX_LENGTH 16384

static TaskHandle_t uart_task_handle = NULL;
static bool uart_running = false;

static void send_response(const char *response)
{
    if (response) {
        uart_write_bytes(UART_NUM, response, strlen(response));
        uart_write_bytes(UART_NUM, "\r\n", 2);
    }
}

static void uart_command_task(void *pvParameters)
{
    uint8_t data[UART_BUF_SIZE];
    char *command_buffer = heap_caps_malloc(CMD_MAX_LENGTH, MALLOC_CAP_8BIT);
    if (!command_buffer) {
        ESP_LOGE(TAG, "Failed to allocate command buffer");
        vTaskDelete(NULL);
        return;
    }
    size_t cmd_pos = 0;

    ESP_LOGI(TAG, "UART command task started");

    while (uart_running) {
        int len = uart_read_bytes(UART_NUM, data, UART_BUF_SIZE, pdMS_TO_TICKS(100));
        
        for (int i = 0; i < len; i++) {
            char c = (char)data[i];
            
            // Handle newline (command complete)
            if (c == '\n' || c == '\r') {
                if (cmd_pos > 0) {
                    command_buffer[cmd_pos] = '\0';
                    ESP_LOGI(TAG, "RX: %s", command_buffer);
                    
                    // Execute command
                    commands_execute(command_buffer, send_response);
                    
                    cmd_pos = 0;
                }
            }
            // Handle backspace
            else if (c == '\b' || c == 127) {
                if (cmd_pos > 0) {
                    cmd_pos--;
                }
            }
            // Add character to buffer
            else if (cmd_pos < CMD_MAX_LENGTH - 1 && c >= 32 && c < 127) {
                command_buffer[cmd_pos++] = c;
            }
        }
    }

    free(command_buffer);
    ESP_LOGI(TAG, "UART command task stopped");
    vTaskDelete(NULL);
}

esp_err_t uart_commands_init(void)
{
    ESP_LOGI(TAG, "Initializing UART command interface");
    ESP_LOGI(TAG, "CONFIG_UART_COMMANDS_PORT_NUM=%d, UART_NUM=%d", CONFIG_UART_COMMANDS_PORT_NUM, (int)UART_NUM);

    // Reclaim UART0 from ROM/bootloader if needed
    if (UART_NUM == UART_NUM_0) {
        uart_driver_delete(UART_NUM_0);
    }

    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 0, NULL, 0));

    ESP_LOGI(TAG, "UART%d configured: %d baud, TX=%d, RX=%d", UART_NUM, UART_BAUD_RATE, UART_TX_PIN, UART_RX_PIN);
    
    return ESP_OK;
}

esp_err_t uart_commands_start(void)
{
    if (uart_running) {
        ESP_LOGW(TAG, "UART command task already running");
        return ESP_OK;
    }

    uart_running = true;

    BaseType_t ret = xTaskCreate(uart_command_task, "uart_cmd", 16384, NULL, 5, &uart_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART command task");
        uart_running = false;
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t uart_commands_stop(void)
{
    if (!uart_running) {
        return ESP_OK;
    }

    uart_running = false;

    if (uart_task_handle) {
        uart_task_handle = NULL;
    }

    ESP_LOGI(TAG, "UART command task stopped");
    return ESP_OK;
}

#else // CONFIG_UART_COMMANDS_ENABLED not defined

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

#endif // CONFIG_UART_COMMANDS_ENABLED
