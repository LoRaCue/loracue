idf_component_register(
    SRCS "bsp_heltec_v3.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc esp_driver_spi esp_driver_i2c spi_flash
)
