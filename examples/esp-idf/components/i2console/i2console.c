/**
 * @file i2console.c
 * @brief I2Console ESP-IDF Component Implementation
 */

#include "i2console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

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
    i2c_port_t port;
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
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2console.addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2console.addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2console.port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Write data to I2Console
 */
static esp_err_t i2console_write_data(const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2console.addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, REG_DATA_START, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2console.port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
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

esp_err_t i2console_init(i2c_port_t port, uint8_t addr)
{
    i2console.port = port;
    i2console.addr = addr;
    
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
