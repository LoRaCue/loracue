/**
 * @file bsp_lilygo_t5.c
 * @brief BSP for LilyGO T5 4.7" E-Paper (ED047TC1)
 */

#include "bq25896.h"
#include "bq27220.h"
#include "bsp.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "gt911.h"
#include "lvgl.h"
#include "pca9535.h"
#include "pcf85063.h"
#include "tps65185.h"

static const char *TAG = "BSP_LILYGO_T5";

// Display specifications
#define EPAPER_WIDTH 960
#define EPAPER_HEIGHT 540
#define EPAPER_GRAYSCALE 16

// I2C Configuration (T5 Pro uses GPIO40/39, not 17/18!)
#define BOARD_I2C_PORT I2C_NUM_0
#define BOARD_SCL GPIO_NUM_40
#define BOARD_SDA GPIO_NUM_39
#define PCA9535_ADDR 0x20

// SPI Configuration (shared by LoRa and SD Card)
#define BOARD_SPI_MISO GPIO_NUM_21
#define BOARD_SPI_MOSI GPIO_NUM_13
#define BOARD_SPI_SCLK GPIO_NUM_14

// Touch (GT911 on I2C)
#define BOARD_TOUCH_INT GPIO_NUM_3
#define BOARD_TOUCH_RST GPIO_NUM_9

// LoRa SX1262 (SPI)
#define BOARD_LORA_CS GPIO_NUM_46
#define BOARD_LORA_IRQ GPIO_NUM_10
#define BOARD_LORA_RST GPIO_NUM_1
#define BOARD_LORA_BUSY GPIO_NUM_47

// SD Card (SPI)
#define BOARD_SD_CS GPIO_NUM_12

// E-Paper Parallel Interface
#define EP_D0 GPIO_NUM_5
#define EP_D1 GPIO_NUM_6
#define EP_D2 GPIO_NUM_7
#define EP_D3 GPIO_NUM_15
#define EP_D4 GPIO_NUM_16
#define EP_D5 GPIO_NUM_17
#define EP_D6 GPIO_NUM_18
#define EP_D7 GPIO_NUM_8
#define EP_CKV GPIO_NUM_48
#define EP_STH GPIO_NUM_41
#define EP_LEH GPIO_NUM_42
#define EP_STV GPIO_NUM_45
#define EP_CKH GPIO_NUM_4

// PCA9535 pin mapping for E-Paper control
#define PCA_EP_OE PCA9535_IO10
#define PCA_EP_MODE PCA9535_IO11
#define PCA_BUTTON PCA9535_IO12
#define PCA_TPS_PWRUP PCA9535_IO13
#define PCA_VCOM_CTRL PCA9535_IO14
#define PCA_TPS_WAKEUP PCA9535_IO15
#define PCA_TPS_PWR_GOOD PCA9535_IO16
#define PCA_TPS_INT PCA9535_IO17

// Other
#define BOARD_BL_EN GPIO_NUM_11
#define BOARD_PCA9535_INT GPIO_NUM_38
#define BOARD_BOOT_BTN GPIO_NUM_0
#define BOARD_RTC_IRQ GPIO_NUM_2
#define BOARD_GPS_RXD GPIO_NUM_44
#define BOARD_GPS_TXD GPIO_NUM_43

// LVGL display configuration
#define LVGL_DISPLAY_WIDTH 540
#define LVGL_DISPLAY_HEIGHT 960
#define LVGL_BUFFER_LINES 10      // Reduced to fit in DRAM (balance memory vs performance)
#define LVGL_TASK_STACK_SIZE 8192 // Increased for LVGL operations
#define LVGL_TASK_PRIORITY 5      // Medium priority (below critical tasks)

static lv_disp_t *disp = NULL;
static lv_color_t buf1[LVGL_DISPLAY_WIDTH * LVGL_BUFFER_LINES];
static lv_color_t buf2[LVGL_DISPLAY_WIDTH * LVGL_BUFFER_LINES];
static SemaphoreHandle_t lvgl_mutex = NULL;

static void lvgl_task(void *arg);

static uint32_t lvgl_tick_get(void)
{
    return esp_timer_get_time() / 1000;
}

static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    if (!area || !px_map) {
        ESP_LOGE(TAG, "Invalid flush parameters: area=%p, px_map=%p", area, px_map);
        lv_display_flush_ready(disp_drv);
        return;
    }

    // Calculate flush area
    int32_t width  = area->x2 - area->x1 + 1;
    int32_t height = area->y2 - area->y1 + 1;

    ESP_LOGD(TAG, "Flushing area: (%d,%d) to (%d,%d), size: %dx%d", area->x1, area->y1, area->x2, area->y2, width,
             height);

    // TODO: Implement actual e-paper transfer
    // For now, just acknowledge the flush
    // Real implementation would:
    // 1. Convert LVGL buffer to e-paper format
    // 2. Transfer to ED047TC1 via SPI
    // 3. Trigger display update

    lv_display_flush_ready(disp_drv);
}

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing BSP for LilyGO T5 4.7\" E-Paper");
    esp_err_t ret;

    // Initialize LVGL
    ESP_LOGD(TAG, "Initializing LVGL library");
    lv_init();
    lv_tick_set_cb(lvgl_tick_get);

    // Create LVGL mutex for thread safety
    ESP_LOGD(TAG, "Creating LVGL mutex");
    lvgl_mutex = xSemaphoreCreateMutex();
    if (!lvgl_mutex) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex - out of memory");
        return ESP_ERR_NO_MEM;
    }

    // Create display
    ESP_LOGD(TAG, "Creating LVGL display (%dx%d)", LVGL_DISPLAY_WIDTH, LVGL_DISPLAY_HEIGHT);
    disp = lv_display_create(LVGL_DISPLAY_WIDTH, LVGL_DISPLAY_HEIGHT);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to create LVGL display - lv_display_create returned NULL");
        vSemaphoreDelete(lvgl_mutex);
        lvgl_mutex = NULL;
        return ESP_ERR_NO_MEM;
    }
    lv_display_set_flush_cb(disp, disp_flush);

    // Set draw buffers (using static allocation)
    ESP_LOGD(TAG, "Configuring draw buffers (%d lines, %zu bytes each)", LVGL_BUFFER_LINES, sizeof(buf1));
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG, "LVGL initialized (%dx%d, %d-line buffers)", LVGL_DISPLAY_WIDTH, LVGL_DISPLAY_HEIGHT,
             LVGL_BUFFER_LINES);

    // Initialize I2C bus (if not already initialized)
    if (bsp_i2c_get_bus() == NULL) {
        ESP_LOGD(TAG, "Initializing I2C (SDA=%d, SCL=%d, 400kHz)", BOARD_SDA, BOARD_SCL);
        ret = bsp_i2c_init(BOARD_SDA, BOARD_SCL, 400000);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
            goto cleanup;
        }
    } else {
        ESP_LOGI(TAG, "I2C bus already initialized, skipping");
    }

    // Initialize PCA9535 GPIO expander
    ESP_ERROR_CHECK(pca9535_init(PCA9535_ADDR));

    // Initialize TPS65185 E-Paper power driver
    ESP_ERROR_CHECK(tps65185_init());

    // Initialize GT911 touch controller
    ESP_ERROR_CHECK(gt911_init(BOARD_TOUCH_INT, BOARD_TOUCH_RST));

    // Initialize battery management
    bq25896_init(BOARD_I2C_PORT);
    bq27220_init(BOARD_I2C_PORT);

    // Initialize RTC
    pcf85063_init();

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
    spi_bus_config_t buscfg = {.mosi_io_num     = BOARD_SPI_MOSI,
                               .miso_io_num     = BOARD_SPI_MISO,
                               .sclk_io_num     = BOARD_SPI_SCLK,
                               .quadwp_io_num   = -1,
                               .quadhd_io_num   = -1,
                               .max_transfer_sz = 4096};
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Initialize E-Paper parallel pins
    gpio_config_t io_conf = {.pin_bit_mask = (1ULL << EP_D0) | (1ULL << EP_D1) | (1ULL << EP_D2) | (1ULL << EP_D3) |
                                             (1ULL << EP_D4) | (1ULL << EP_D5) | (1ULL << EP_D6) | (1ULL << EP_D7) |
                                             (1ULL << EP_CKV) | (1ULL << EP_STH) | (1ULL << EP_LEH) | (1ULL << EP_STV) |
                                             (1ULL << EP_CKH),
                             .mode         = GPIO_MODE_OUTPUT,
                             .pull_up_en   = GPIO_PULLUP_DISABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type    = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    // Touch interrupt
    io_conf.pin_bit_mask = (1ULL << BOARD_TOUCH_INT);
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    // Touch reset
    io_conf.pin_bit_mask = (1ULL << BOARD_TOUCH_RST);
    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "LilyGO T5 initialized: %dx%d, %d grayscale", EPAPER_WIDTH, EPAPER_HEIGHT, EPAPER_GRAYSCALE);

    // Start LVGL task handler
    ESP_LOGD(TAG, "Creating LVGL task (stack=%d, priority=%d)", LVGL_TASK_STACK_SIZE, LVGL_TASK_PRIORITY);
    BaseType_t task_ret = xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task - xTaskCreate returned %d", task_ret);
        ret = ESP_FAIL;
        goto cleanup;
    }

    ESP_LOGI(TAG, "BSP initialization complete");
    return ESP_OK;

cleanup:
    ESP_LOGW(TAG, "Cleaning up after initialization failure");
    if (disp) {
        lv_deinit();
        disp = NULL;
    }
    if (lvgl_mutex) {
        vSemaphoreDelete(lvgl_mutex);
        lvgl_mutex = NULL;
    }
    return ret;
}

static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started");
    while (1) {
        if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_task_handler();
            xSemaphoreGive(lvgl_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t bsp_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing BSP");

    // Clean up LVGL resources
    if (disp) {
        lv_deinit();
        disp = NULL;
    }

    // Delete mutex
    if (lvgl_mutex) {
        vSemaphoreDelete(lvgl_mutex);
        lvgl_mutex = NULL;
    }

    ESP_LOGI(TAG, "BSP deinitialized");
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
    gpio_config_t io_conf = {.pin_bit_mask = (1ULL << BOARD_BOOT_BTN),
                             .mode         = GPIO_MODE_INPUT,
                             .pull_up_en   = GPIO_PULLUP_ENABLE,
                             .intr_type    = GPIO_INTR_DISABLE};
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
    gpio_config_t io_conf = {.pin_bit_mask = (1ULL << BOARD_BL_EN),
                             .mode         = GPIO_MODE_OUTPUT,
                             .pull_up_en   = GPIO_PULLUP_DISABLE,
                             .intr_type    = GPIO_INTR_DISABLE};
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

const char *bsp_get_board_name(void)
{
    return "LilyGO T5";
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

esp_err_t bsp_i2c_init_default(void)
{
    return bsp_i2c_init(BOARD_SDA, BOARD_SCL, CONFIG_BSP_I2C_CLOCK_SPEED_HZ);
}

const char *bsp_get_board_id(void)
{
    return "lilygo_t5";
}

const char *bsp_get_model_name(void)
{
    return "LC-Gamma";
}

// USB configuration for LilyGO T5
const bsp_usb_config_t *bsp_get_usb_config(void)
{
    static bsp_usb_config_t usb_config = {.usb_pid = 0x4004, .usb_product = NULL};

    if (!usb_config.usb_product) {
        usb_config.usb_product = bsp_get_model_name();
    }

    return &usb_config;
}

esp_err_t bsp_get_serial_number(char *serial_number, size_t max_len)
{
    if (!serial_number || max_len < 13) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(serial_number, max_len, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return ESP_OK;
}

const bsp_lora_pins_t *bsp_get_lora_pins(void)
{
    static const bsp_lora_pins_t lora_pins = {
        .miso = 21, .mosi = 13, .sclk = 14, .cs = 46, .rst = 1, .busy = 47, .dio1 = 10};
    return &lora_pins;
}

float bsp_read_battery(void)
{
    // Convert from percentage to float (0.0-100.0)
    uint8_t percentage = bsp_battery_get_percentage();
    return (float)percentage;
}

bool bsp_read_button(bsp_button_t button)
{
    return bsp_button_get(button);
}

esp_err_t bsp_validate_hardware(void)
{
    ESP_LOGI(TAG, "Validating LilyGO T5 hardware...");

    // Check I2C devices
    // TODO: Add actual hardware validation

    ESP_LOGI(TAG, "Hardware validation complete");
    return ESP_OK;
}

esp_err_t bsp_set_display_brightness(uint8_t brightness)
{
    // LilyGO T5 E-Paper doesn't have adjustable brightness
    // Could control backlight if available, or adjust refresh intensity
    ESP_LOGD(TAG, "E-Paper brightness control not implemented (value: %d)", brightness);
    return ESP_OK;
}

esp_err_t bsp_display_wake(void)
{
    // E-Paper doesn't have power save mode like OLED
    return ESP_OK;
}

lv_disp_t *bsp_display_get_lvgl_disp(void)
{
    return disp;
}

esp_err_t bsp_get_uart_pins(int uart_num, int *tx_pin, int *rx_pin)
{
    if (!tx_pin || !rx_pin) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (uart_num) {
        case 0:
            *tx_pin = 43; // ESP32-S3 UART0 (USB-JTAG-Serial)
            *rx_pin = 44;
            return ESP_OK;
        case 1:
            *tx_pin = 2; // LilyGO T5 available pins
            *rx_pin = 3;
            return ESP_OK;
        default:
            return ESP_ERR_INVALID_ARG;
    }
}
