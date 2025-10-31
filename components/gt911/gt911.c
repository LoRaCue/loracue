#include "gt911.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "gt911";
static i2c_port_t gt_i2c_port;
static gpio_num_t gt_int_pin;
static gpio_num_t gt_rst_pin;

#define GT911_REG_STATUS    0x814E
#define GT911_REG_POINT1    0x814F

static esp_err_t gt911_write_reg(uint16_t reg, uint8_t *data, size_t len)
{
    uint8_t buf[len + 2];
    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;
    memcpy(&buf[2], data, len);
    return i2c_master_write_to_device(gt_i2c_port, GT911_ADDR, buf, len + 2, pdMS_TO_TICKS(100));
}

static esp_err_t gt911_read_reg(uint16_t reg, uint8_t *data, size_t len)
{
    uint8_t reg_buf[2] = {reg >> 8, reg & 0xFF};
    return i2c_master_write_read_device(gt_i2c_port, GT911_ADDR, reg_buf, 2, data, len, pdMS_TO_TICKS(100));
}

esp_err_t gt911_init(i2c_port_t i2c_port, gpio_num_t int_pin, gpio_num_t rst_pin)
{
    gt_i2c_port = i2c_port;
    gt_int_pin = int_pin;
    gt_rst_pin = rst_pin;
    
    ESP_LOGI(TAG, "Initializing GT911 touch controller");
    
    // Reset sequence
    gpio_set_level(gt_rst_pin, 0);
    gpio_set_level(gt_int_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(gt_int_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(gt_rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    return ESP_OK;
}

esp_err_t gt911_read_touch(gt911_touch_point_t *point, uint8_t *num_points)
{
    uint8_t status;
    esp_err_t ret = gt911_read_reg(GT911_REG_STATUS, &status, 1);
    if (ret != ESP_OK) return ret;
    
    *num_points = status & 0x0F;
    if (*num_points == 0 || *num_points > 5) {
        // Clear status
        uint8_t clear = 0;
        gt911_write_reg(GT911_REG_STATUS, &clear, 1);
        return ESP_OK;
    }
    
    // Read first touch point
    uint8_t data[8];
    ret = gt911_read_reg(GT911_REG_POINT1, data, 8);
    if (ret == ESP_OK) {
        point->id = data[0];
        point->x = data[1] | (data[2] << 8);
        point->y = data[3] | (data[4] << 8);
        point->size = data[5] | (data[6] << 8);
    }
    
    // Clear status
    uint8_t clear = 0;
    gt911_write_reg(GT911_REG_STATUS, &clear, 1);
    
    return ret;
}
