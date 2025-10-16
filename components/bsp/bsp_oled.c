/**
 * @file bsp_oled.c
 * @brief OLED display u8g2 HAL callbacks
 */

#include "bsp.h"
#include "u8g2.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BSP_OLED";
static const unsigned int I2C_TIMEOUT_MS = 1000;

static i2c_master_dev_handle_t oled_i2c_dev = NULL;
static uint8_t i2c_buffer[256];
static size_t i2c_buffer_len = 0;

static gpio_num_t oled_rst_pin = GPIO_NUM_NC;

uint8_t bsp_u8g2_i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
    case U8X8_MSG_BYTE_INIT: {
        // I2C bus already initialized by BSP
        ESP_LOGD(TAG, "U8X8_MSG_BYTE_INIT");
        break;
    }

    case U8X8_MSG_BYTE_SEND: {
        uint8_t *data_ptr = (uint8_t *)arg_ptr;
        ESP_LOG_BUFFER_HEXDUMP(TAG, data_ptr, arg_int, ESP_LOG_VERBOSE);

        // Buffer the data
        if (i2c_buffer_len + arg_int <= sizeof(i2c_buffer)) {
            memcpy(i2c_buffer + i2c_buffer_len, data_ptr, arg_int);
            i2c_buffer_len += arg_int;
        } else {
            ESP_LOGE(TAG, "I2C buffer overflow");
        }
        break;
    }

    case U8X8_MSG_BYTE_START_TRANSFER: {
        uint8_t i2c_address = u8x8_GetI2CAddress(u8x8) >> 1;
        ESP_LOGD(TAG, "Start I2C transfer to 0x%02X", i2c_address);

        // Reset buffer
        i2c_buffer_len = 0;

        // Add device if not already added
        if (oled_i2c_dev == NULL) {
            esp_err_t ret = bsp_i2c_add_device(i2c_address, 400000, &oled_i2c_dev);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to add OLED I2C device: %s", esp_err_to_name(ret));
            }
        }
        break;
    }

    case U8X8_MSG_BYTE_END_TRANSFER: {
        ESP_LOGD(TAG, "End I2C transfer: %d bytes", i2c_buffer_len);

        // Transmit buffered data
        if (i2c_buffer_len > 0 && oled_i2c_dev != NULL) {
            esp_err_t ret = i2c_master_transmit(oled_i2c_dev, i2c_buffer, i2c_buffer_len, I2C_TIMEOUT_MS);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "I2C transmit failed: %s", esp_err_to_name(ret));
            }
        }
        break;
    }

    default:
        return 0;
    }

    return 1;
}

uint8_t bsp_u8g2_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    (void)u8x8;
    (void)arg_ptr;

    switch (msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        // Initialize reset pin if defined
        if (oled_rst_pin != GPIO_NUM_NC) {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << oled_rst_pin),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE,
            };
            gpio_config(&io_conf);
        }
        break;

    case U8X8_MSG_DELAY_MILLI:
        vTaskDelay(pdMS_TO_TICKS(arg_int));
        break;

    case U8X8_MSG_DELAY_10MICRO:
        esp_rom_delay_us(arg_int * 10);
        break;

    case U8X8_MSG_DELAY_100NANO:
        esp_rom_delay_us(1);
        break;

    case U8X8_MSG_GPIO_RESET:
        if (oled_rst_pin != GPIO_NUM_NC) {
            gpio_set_level(oled_rst_pin, arg_int);
        }
        break;

    default:
        return 0;
    }

    return 1;
}

void bsp_oled_set_reset_pin(gpio_num_t pin)
{
    oled_rst_pin = pin;
}
