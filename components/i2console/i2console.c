/**
 * @file i2console.c
 * @brief I2Console ESP-IDF Component Implementation
 */

#include "i2console.h"
#include "bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdarg.h>

static const char *TAG = "i2console";

// I2Console register map
#define REG_DEVICE_ID       0x00
#define REG_FW_VERSION      0x01
#define REG_I2C_ADDRESS     0x02
#define REG_CLOCK_STRETCH   0x03
#define REG_TX_AVAIL_LOW    0x10
#define REG_TX_AVAIL_HIGH   0x11
#define REG_RX_AVAIL        0x12
#define REG_DATA_START      0x20
#define REG_VERSION_STRING  0x04

// Component state
static struct {
    i2c_master_dev_handle_t dev_handle;
    uint8_t addr;
    bool connected;
    QueueHandle_t tx_queue;
} i2console = {
    .connected = false,
};

#define TX_QUEUE_SIZE 10
#define TX_BUFFER_SIZE 256

typedef struct {
    char data[TX_BUFFER_SIZE];
    size_t len;
} tx_msg_t;

/**
 * @brief Read I2Console register
 */
static esp_err_t i2console_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(i2console.dev_handle, &reg, 1, data, len, 100);
}

/**
 * @brief Write data to I2Console
 */
static esp_err_t i2console_write_data(const uint8_t *data, size_t len)
{
    uint8_t tx_buffer[len + 1];
    tx_buffer[0] = REG_DATA_START;
    memcpy(&tx_buffer[1], data, len);
    return i2c_master_transmit(i2console.dev_handle, tx_buffer, len + 1, 100);
}

/**
 * @brief Detect I2Console device
 */
static bool i2console_detect(void)
{
    uint8_t device_id[2];
    if (i2console_read_reg(REG_DEVICE_ID, device_id, 2) != ESP_OK) {
        return false;
    }
    
    uint16_t id = (device_id[0] << 8) | device_id[1];
    return (id == I2CONSOLE_DEVICE_ID);
}

/**
 * @brief TX task - sends queued data to I2Console
 */
static void i2console_tx_task(void *arg)
{
    tx_msg_t msg;
    
    while (1) {
        if (xQueueReceive(i2console.tx_queue, &msg, portMAX_DELAY) == pdTRUE) {
            if (i2console.connected) {
                i2console_write_data((uint8_t *)msg.data, msg.len);
            }
        }
    }
}

/**
 * @brief Custom vprintf for ESP-IDF logging
 */
static int i2console_vprintf(const char *fmt, va_list args)
{
    // Format to buffer
    char buffer[TX_BUFFER_SIZE];
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    if (len > 0 && len < sizeof(buffer)) {
        // Queue for transmission
        tx_msg_t msg;
        msg.len = (len < TX_BUFFER_SIZE) ? len : TX_BUFFER_SIZE;
        memcpy(msg.data, buffer, msg.len);
        xQueueSend(i2console.tx_queue, &msg, 0);  // Non-blocking
    }
    
    // Also output to default (UART)
    return vprintf(fmt, args);
}

esp_err_t i2console_init(uint8_t addr)
{
    i2console.addr = addr;
    
    // Add I2Console device to BSP I2C bus
    esp_err_t ret = bsp_i2c_add_device(addr, 400000, &i2console.dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add I2Console device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Detect device
    if (!i2console_detect()) {
        ESP_LOGW(TAG, "I2Console not detected at 0x%02X", addr);
        return ESP_ERR_NOT_FOUND;
    }
    
    i2console.connected = true;
    ESP_LOGI(TAG, "I2Console detected at 0x%02X", addr);
    
    // Create TX queue
    i2console.tx_queue = xQueueCreate(TX_QUEUE_SIZE, sizeof(tx_msg_t));
    if (!i2console.tx_queue) {
        ESP_LOGE(TAG, "Failed to create TX queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Start TX task
    xTaskCreate(i2console_tx_task, "i2console_tx", 2048, NULL, 5, NULL);
    
    // Register as log output
    esp_log_set_vprintf(i2console_vprintf);
    
    ESP_LOGI(TAG, "I2Console initialized - logs will be mirrored");
    
    return ESP_OK;
}

esp_err_t i2console_write(const char *data, size_t len)
{
    if (!i2console.connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    tx_msg_t msg;
    msg.len = (len < TX_BUFFER_SIZE) ? len : TX_BUFFER_SIZE;
    memcpy(msg.data, data, msg.len);
    
    if (xQueueSend(i2console.tx_queue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

bool i2console_is_connected(void)
{
    return i2console.connected;
}

esp_err_t i2console_get_version(char *version)
{
    if (!i2console.connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t ver_data[16];
    esp_err_t ret = i2console_read_reg(REG_VERSION_STRING, ver_data, sizeof(ver_data));
    if (ret == ESP_OK) {
        memcpy(version, ver_data, 16);
        version[15] = '\0';  // Ensure null-terminated
    }
    
    return ret;
}
