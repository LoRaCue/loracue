#include "pcf85063.h"
#include "bsp.h" // For bsp_i2c_add_device
#include "esp_log.h"

static const char *TAG                    = "pcf85063";
static i2c_master_dev_handle_t rtc_handle = NULL;

#define PCF85063_REG_SECONDS 0x04
#define PCF85063_I2C_FREQ_HZ 400000
#define I2C_TIMEOUT_MS 100
#define BCD_MASK_LOW 0x0F
#define MASK_SEC_MIN 0x7F
#define MASK_HOUR_DAY 0x3F
#define MASK_WEEKDAY 0x07
#define MASK_MONTH 0x1F
#define YEAR_OFFSET 100

static uint8_t bcd_to_dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & BCD_MASK_LOW);
}
static uint8_t dec_to_bcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

static esp_err_t pcf85063_write_regs(uint8_t reg, const uint8_t *data, size_t len)
{
    if (!rtc_handle)
        return ESP_ERR_INVALID_STATE;
    uint8_t buf[len + 1];
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    return i2c_master_transmit(rtc_handle, buf, len + 1, I2C_TIMEOUT_MS);
}

static esp_err_t pcf85063_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    if (!rtc_handle)
        return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit_receive(rtc_handle, &reg, 1, data, len, I2C_TIMEOUT_MS);
}

esp_err_t pcf85063_init(void)
{
    ESP_LOGI(TAG, "Initializing PCF85063 RTC");
    // Default to 400kHz
    return bsp_i2c_add_device(PCF85063_ADDR, PCF85063_I2C_FREQ_HZ, &rtc_handle);
}

esp_err_t pcf85063_set_time(const struct tm *time)
{
    uint8_t data[7];
    data[0] = dec_to_bcd(time->tm_sec);
    data[1] = dec_to_bcd(time->tm_min);
    data[2] = dec_to_bcd(time->tm_hour);
    data[3] = dec_to_bcd(time->tm_mday);
    data[4] = time->tm_wday;
    data[5] = dec_to_bcd(time->tm_mon + 1);
    data[6] = dec_to_bcd(time->tm_year - YEAR_OFFSET);
    return pcf85063_write_regs(PCF85063_REG_SECONDS, data, 7);
}

esp_err_t pcf85063_get_time(struct tm *time)
{
    uint8_t data[7];
    esp_err_t ret = pcf85063_read_regs(PCF85063_REG_SECONDS, data, 7);
    if (ret == ESP_OK) {
        time->tm_sec  = bcd_to_dec(data[0] & MASK_SEC_MIN);
        time->tm_min  = bcd_to_dec(data[1] & MASK_SEC_MIN);
        time->tm_hour = bcd_to_dec(data[2] & MASK_HOUR_DAY);
        time->tm_mday = bcd_to_dec(data[3] & MASK_HOUR_DAY);
        time->tm_wday = data[4] & MASK_WEEKDAY;
        time->tm_mon  = bcd_to_dec(data[5] & MASK_MONTH) - 1;
        time->tm_year = bcd_to_dec(data[6]) + YEAR_OFFSET;
    }
    return ret;
}
