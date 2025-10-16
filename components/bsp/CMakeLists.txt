# Conditional BSP compilation based on build target
if(CMAKE_C_FLAGS MATCHES "SIMULATOR_BUILD" OR SIMULATOR_BUILD)
    set(BSP_SRCS "bsp_wokwi.c" "bsp_i2c.c" "bsp_oled.c")
    message(STATUS "Building with Wokwi BSP")
else()
    set(BSP_SRCS "bsp_heltec_v3.c" "bsp_i2c.c" "bsp_oled.c")
    message(STATUS "Building with Heltec V3 BSP")
endif()

idf_component_register(
    SRCS ${BSP_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc esp_driver_spi esp_driver_i2c spi_flash u8g2
)
