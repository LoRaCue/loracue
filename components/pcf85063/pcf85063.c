#include "pcf85063.h"
#include "esp_log.h"

static const char *TAG = "pcf85063";
static i2c_port_t rtc_i2c_port;

#define PCF85063_REG_SECONDS 0x04

static uint8_t bcd_to_dec(uint8_t val) { return (val >> 4) * 10 + (val & 0x0F); }
static uint8_t dec_to_bcd(uint8_t val) { return ((val / 10) << 4) | (val % 10); }

static esp_err_t pcf85063_write_regs(uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t buf[len + 1];
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    return i2c_master_write_to_device(rtc_i2c_port, PCF85063_ADDR, buf, len + 1, pdMS_TO_TICKS(100));
}

static esp_err_t pcf85063_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(rtc_i2c_port, PCF85063_ADDR, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

esp_err_t pcf85063_init(i2c_port_t i2c_port)
{
    rtc_i2c_port = i2c_port;
    ESP_LOGI(TAG, "Initializing PCF85063 RTC");
    return ESP_OK;
}

esp_err_t pcf85063_set_time(struct tm *time)
{
    uint8_t data[7];
    data[0] = dec_to_bcd(time->tm_sec);
    data[1] = dec_to_bcd(time->tm_min);
    data[2] = dec_to_bcd(time->tm_hour);
    data[3] = dec_to_bcd(time->tm_mday);
    data[4] = time->tm_wday;
    data[5] = dec_to_bcd(time->tm_mon + 1);
    data[6] = dec_to_bcd(time->tm_year - 100);
    return pcf85063_write_regs(PCF85063_REG_SECONDS, data, 7);
}

esp_err_t pcf85063_get_time(struct tm *time)
{
    uint8_t data[7];
    esp_err_t ret = pcf85063_read_regs(PCF85063_REG_SECONDS, data, 7);
    if (ret == ESP_OK) {
        time->tm_sec = bcd_to_dec(data[0] & 0x7F);
        time->tm_min = bcd_to_dec(data[1] & 0x7F);
        time->tm_hour = bcd_to_dec(data[2] & 0x3F);
        time->tm_mday = bcd_to_dec(data[3] & 0x3F);
        time->tm_wday = data[4] & 0x07;
        time->tm_mon = bcd_to_dec(data[5] & 0x1F) - 1;
        time->tm_year = bcd_to_dec(data[6]) + 100;
    }
    return ret;
}
