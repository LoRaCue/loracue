/**
 * @file bsp_lilygo_t5.c
 * @brief BSP for LilyGO T5 4.7" E-Paper (ED047TC1)
 */

#include "bsp.h"
#include "pca9535.h"
#include "tps65185.h"
#include "gt911.h"
#include "bq25896.h"
#include "bq27220.h"
#include "pcf85063.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "lvgl.h"

static const char *TAG = "bsp_lilygo_t5";

// Display specifications
#define EPAPER_WIDTH  960
#define EPAPER_HEIGHT 540
#define EPAPER_GRAYSCALE 16

// I2C Configuration
#define BOARD_I2C_PORT      I2C_NUM_0
#define BOARD_SCL           GPIO_NUM_40
#define BOARD_SDA           GPIO_NUM_39
#define PCA9535_ADDR        0x20

// SPI Configuration
#define BOARD_SPI_MISO      GPIO_NUM_21
#define BOARD_SPI_MOSI      GPIO_NUM_13
#define BOARD_SPI_SCLK      GPIO_NUM_14

// Touch (I2C)
#define BOARD_TOUCH_INT     GPIO_NUM_3
#define BOARD_TOUCH_RST     GPIO_NUM_9

// LoRa (SPI)
#define BOARD_LORA_CS       GPIO_NUM_46
#define BOARD_LORA_IRQ      GPIO_NUM_10
#define BOARD_LORA_RST      GPIO_NUM_1
#define BOARD_LORA_BUSY     GPIO_NUM_47

// SD Card (SPI)
#define BOARD_SD_CS         GPIO_NUM_12

// E-Paper Parallel Interface
#define EP_D0               GPIO_NUM_5
#define EP_D1               GPIO_NUM_6
#define EP_D2               GPIO_NUM_7
#define EP_D3               GPIO_NUM_15
#define EP_D4               GPIO_NUM_16
#define EP_D5               GPIO_NUM_17
#define EP_D6               GPIO_NUM_18
#define EP_D7               GPIO_NUM_8
#define EP_CKV              GPIO_NUM_48
#define EP_STH              GPIO_NUM_41
#define EP_LEH              GPIO_NUM_42
#define EP_STV              GPIO_NUM_45
#define EP_CKH              GPIO_NUM_4

// PCA9535 pin mapping for E-Paper control
#define PCA_EP_OE           PCA9535_IO10
#define PCA_EP_MODE         PCA9535_IO11
#define PCA_BUTTON          PCA9535_IO12
#define PCA_TPS_PWRUP       PCA9535_IO13
#define PCA_VCOM_CTRL       PCA9535_IO14
#define PCA_TPS_WAKEUP      PCA9535_IO15
#define PCA_TPS_PWR_GOOD    PCA9535_IO16
#define PCA_TPS_INT         PCA9535_IO17

// Other
#define BOARD_BL_EN         GPIO_NUM_11
#define BOARD_PCA9535_INT   GPIO_NUM_38
#define BOARD_BOOT_BTN      GPIO_NUM_0
#define BOARD_RTC_IRQ       GPIO_NUM_2

static spi_device_handle_t spi_handle;
static lv_disp_t *disp;

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing BSP for LilyGO T5 4.7\" E-Paper");
    
    // Initialize I2C
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BOARD_SDA,
        .scl_io_num = BOARD_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    ESP_ERROR_CHECK(i2c_param_config(BOARD_I2C_PORT, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(BOARD_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));
    
    // Initialize PCA9535 GPIO expander
    ESP_ERROR_CHECK(pca9535_init(BOARD_I2C_PORT, PCA9535_ADDR));
    
    // Initialize TPS65185 E-Paper power driver
    ESP_ERROR_CHECK(tps65185_init(BOARD_I2C_PORT));
    
    // Initialize GT911 touch controller
    ESP_ERROR_CHECK(gt911_init(BOARD_I2C_PORT, BOARD_TOUCH_INT, BOARD_TOUCH_RST));
    
    // Initialize battery management
    bq25896_init(BOARD_I2C_PORT);
    bq27220_init(BOARD_I2C_PORT);
    
    // Initialize RTC
    pcf85063_init(BOARD_I2C_PORT);
    
    // Configure PCA9535 pins for E-Paper
    pca9535_set_direction(PCA_EP_OE, true);
    pca9535_set_direction(PCA_EP_MODE, true);
    pca9535_set_direction(PCA_TPS_PWRUP, true);
    pca9535_set_direction(PCA_VCOM_CTRL, true);
    pca9535_set_direction(PCA_TPS_WAKEUP, true);
    pca9535_set_direction(PCA_BUTTON, false);
    pca9535_set_direction(PCA_TPS_PWR_GOOD, false);
    pca9535_set_direction(PCA_TPS_INT, false);
    
    // Initialize SPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = BOARD_SPI_MOSI,
        .miso_io_num = BOARD_SPI_MISO,
        .sclk_io_num = BOARD_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
    // Initialize E-Paper parallel pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EP_D0) | (1ULL << EP_D1) | (1ULL << EP_D2) | (1ULL << EP_D3) |
                        (1ULL << EP_D4) | (1ULL << EP_D5) | (1ULL << EP_D6) | (1ULL << EP_D7) |
                        (1ULL << EP_CKV) | (1ULL << EP_STH) | (1ULL << EP_LEH) | 
                        (1ULL << EP_STV) | (1ULL << EP_CKH),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Touch interrupt
    io_conf.pin_bit_mask = (1ULL << BOARD_TOUCH_INT);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    
    // Touch reset
    io_conf.pin_bit_mask = (1ULL << BOARD_TOUCH_RST);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "LilyGO T5 initialized: %dx%d, %d grayscale", 
             EPAPER_WIDTH, EPAPER_HEIGHT, EPAPER_GRAYSCALE);
    
    return ESP_OK;
}

esp_err_t bsp_epaper_power_on(void)
{
    ESP_LOGI(TAG, "Powering on E-Paper display");
    pca9535_set_output(PCA_TPS_PWRUP, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    pca9535_set_output(PCA_TPS_WAKEUP, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    tps65185_power_on();
    vTaskDelay(pdMS_TO_TICKS(10));
    pca9535_set_output(PCA_EP_OE, 1);
    return ESP_OK;
}

esp_err_t bsp_epaper_power_off(void)
{
    ESP_LOGI(TAG, "Powering off E-Paper display");
    pca9535_set_output(PCA_EP_OE, 0);
    tps65185_power_off();
    pca9535_set_output(PCA_TPS_WAKEUP, 0);
    pca9535_set_output(PCA_TPS_PWRUP, 0);
    return ESP_OK;
}

esp_err_t bsp_init_buttons(void)
{
    // Boot button
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOARD_BOOT_BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "Button initialized (GPIO0)");
    return ESP_OK;
}

esp_err_t bsp_init_battery(void)
{
    ESP_LOGI(TAG, "Battery monitoring not implemented");
    return ESP_OK;
}

esp_err_t bsp_init_led(void)
{
    // Backlight enable
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOARD_BL_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(BOARD_BL_EN, 0);
    
    ESP_LOGI(TAG, "Backlight control initialized");
    return ESP_OK;
}

bool bsp_button_get(bsp_button_t button)
{
    if (button == BSP_BUTTON_PREV) {
        return !gpio_get_level(BOARD_BOOT_BTN);
    }
    return false;
}

uint32_t bsp_battery_get_voltage_mv(void)
{
    return bq27220_get_voltage_mv();
}

uint8_t bsp_battery_get_percentage(void)
{
    return bq27220_get_soc();
}

bool bsp_battery_is_charging(void)
{
    return bq25896_is_charging();
}

void bsp_led_set(bool state)
{
    gpio_set_level(BOARD_BL_EN, state ? 1 : 0);
}

void bsp_power_deep_sleep(uint64_t time_us)
{
    ESP_LOGI(TAG, "Entering deep sleep for %llu us", time_us);
}

const char *bsp_get_board_id(void)
{
    return "lilygo_t5";
}

lv_disp_t *bsp_display_get_lvgl_disp(void)
{
    return disp;
}
