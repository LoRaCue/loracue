# Conditional BSP compilation based on build target
if(CMAKE_C_FLAGS MATCHES "SIMULATOR_BUILD" OR SIMULATOR_BUILD)
    set(BSP_SRCS "bsp_wokwi.c")
    message(STATUS "Building with Wokwi BSP (SSD1306)")
else()
    set(BSP_SRCS "bsp_heltec_v3.c")
    message(STATUS "Building with Heltec V3 BSP (SH1106)")
endif()

idf_component_register(
    SRCS ${BSP_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc esp_driver_spi spi_flash u8g2 u8g2-hal-esp-idf
)
