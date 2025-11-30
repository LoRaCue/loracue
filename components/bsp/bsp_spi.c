/**
 * @file bsp_spi.c
 * @brief Centralized SPI bus management for BSP
 */

#include "bsp.h"
#include "driver/spi_master.h"
#include "esp_log.h"

static const char *TAG = "BSP_SPI";

esp_err_t bsp_spi_init_bus(spi_host_device_t host, int mosi, int miso, int sclk, size_t max_transfer_sz) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sclk,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = max_transfer_sz,
    };

    ESP_LOGI(TAG, "Initializing SPI bus: MOSI=%d, MISO=%d, SCLK=%d, max_transfer=%zu", 
             mosi, miso, sclk, max_transfer_sz);
    return spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
}

esp_err_t bsp_spi_add_device(spi_host_device_t host, int cs, uint32_t clock_hz, 
                             uint8_t mode, uint8_t queue_size, spi_device_handle_t *handle) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = clock_hz,
        .mode = mode,
        .spics_io_num = cs,
        .queue_size = queue_size,
    };

    ESP_LOGI(TAG, "Adding SPI device: CS=%d, clock=%lu Hz, mode=%d", cs, clock_hz, mode);
    return spi_bus_add_device(host, &devcfg, handle);
}
