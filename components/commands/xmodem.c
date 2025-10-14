#include "xmodem.h"
#include "ota_engine.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_crc.h"
#include <string.h>

static const char *TAG = "XMODEM";

#define UART_NUM UART_NUM_0
#define XMODEM_TIMEOUT_MS 10000
#define MAX_RETRIES 10

static uint16_t crc16_xmodem(const uint8_t *data, size_t len)
{
    uint16_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

static int uart_read_byte(uint8_t *byte, int timeout_ms)
{
    return uart_read_bytes(UART_NUM, byte, 1, pdMS_TO_TICKS(timeout_ms));
}

static void uart_send_byte(uint8_t byte)
{
    uart_write_bytes(UART_NUM, &byte, 1);
}

esp_err_t xmodem_receive(size_t expected_size)
{
    uint8_t block[XMODEM_1K_BLOCK_SIZE + 5];
    uint8_t packet_num = 1;
    size_t total_received = 0;
    int retries = 0;
    
    esp_err_t ret = ota_engine_start(expected_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA start failed");
        uart_send_byte(XMODEM_CAN);
        return ret;
    }
    
    ESP_LOGI(TAG, "Starting XMODEM-1K receive, size: %zu", expected_size);
    uart_send_byte(XMODEM_CRC);
    
    while (total_received < expected_size) {
        uint8_t header;
        if (uart_read_byte(&header, XMODEM_TIMEOUT_MS) <= 0) {
            ESP_LOGE(TAG, "Timeout waiting for header");
            if (++retries > MAX_RETRIES) {
                ota_engine_abort();
                uart_send_byte(XMODEM_CAN);
                return ESP_ERR_TIMEOUT;
            }
            uart_send_byte(XMODEM_NAK);
            continue;
        }
        
        if (header == XMODEM_EOT) {
            uart_send_byte(XMODEM_ACK);
            break;
        }
        
        if (header == XMODEM_CAN) {
            ESP_LOGE(TAG, "Transfer cancelled by sender");
            ota_engine_abort();
            return ESP_FAIL;
        }
        
        if (header != XMODEM_STX && header != XMODEM_SOH) {
            ESP_LOGW(TAG, "Invalid header: 0x%02x", header);
            uart_send_byte(XMODEM_NAK);
            continue;
        }
        
        size_t block_size = (header == XMODEM_STX) ? XMODEM_1K_BLOCK_SIZE : XMODEM_BLOCK_SIZE;
        
        uint8_t pkt_num, pkt_num_inv;
        if (uart_read_byte(&pkt_num, 1000) <= 0 || uart_read_byte(&pkt_num_inv, 1000) <= 0) {
            uart_send_byte(XMODEM_NAK);
            continue;
        }
        
        if (pkt_num != packet_num || pkt_num_inv != (uint8_t)~packet_num) {
            ESP_LOGW(TAG, "Packet number mismatch: got %d, expected %d", pkt_num, packet_num);
            uart_send_byte(XMODEM_NAK);
            continue;
        }
        
        size_t read = uart_read_bytes(UART_NUM, block, block_size + 2, pdMS_TO_TICKS(5000));
        if (read != block_size + 2) {
            ESP_LOGW(TAG, "Incomplete block: %zu/%zu", read, block_size + 2);
            uart_send_byte(XMODEM_NAK);
            continue;
        }
        
        uint16_t received_crc = (block[block_size] << 8) | block[block_size + 1];
        uint16_t calculated_crc = crc16_xmodem(block, block_size);
        
        if (received_crc != calculated_crc) {
            ESP_LOGW(TAG, "CRC mismatch: 0x%04x != 0x%04x", received_crc, calculated_crc);
            uart_send_byte(XMODEM_NAK);
            continue;
        }
        
        size_t write_size = block_size;
        if (total_received + write_size > expected_size) {
            write_size = expected_size - total_received;
        }
        
        ret = ota_engine_write(block, write_size);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "OTA write failed");
            ota_engine_abort();
            uart_send_byte(XMODEM_CAN);
            return ret;
        }
        
        total_received += write_size;
        packet_num++;
        retries = 0;
        uart_send_byte(XMODEM_ACK);
        
        if (total_received % 10240 == 0) {
            ESP_LOGI(TAG, "Progress: %zu/%zu bytes", total_received, expected_size);
        }
    }
    
    ret = ota_engine_finish();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA finish failed");
        return ret;
    }
    
    ESP_LOGI(TAG, "XMODEM receive complete: %zu bytes", total_received);
    return ESP_OK;
}
