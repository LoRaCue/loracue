idf_component_register(
    SRCS "bsp_common.c" "bsp_heltec_v3.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc spi_flash
)
