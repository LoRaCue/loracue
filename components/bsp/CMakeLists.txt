# Conditional BSP compilation based on board selection
if(CONFIG_BOARD_LILYGO_T5)
    set(BSP_SRCS "bsp_lilygo_t5.c")
    set(BSP_REQUIRES driver esp_adc esp_driver_spi esp_driver_i2c spi_flash lvgl 
                     pca9535 tps65185 gt911 bq25896 bq27220 pcf85063)
    message(STATUS "Building for LilyGO T5 4.7\" E-Paper (UI_RICH)")
elseif(CONFIG_BOARD_WOKWI)
    set(BSP_SRCS "bsp_wokwi.c" "bsp_i2c.c" "bsp_oled.c")
    set(BSP_REQUIRES driver esp_adc esp_driver_spi esp_driver_i2c spi_flash u8g2)
    message(STATUS "Building for Wokwi Simulator (UI_EMBEDDED)")
elseif(CONFIG_BOARD_HELTEC_V3)
    set(BSP_SRCS "bsp_heltec_v3.c" "bsp_i2c.c" "bsp_oled.c")
    set(BSP_REQUIRES driver esp_adc esp_driver_spi esp_driver_i2c spi_flash u8g2)
    message(STATUS "Building for Heltec V3 (UI_EMBEDDED)")
else()
    # Fallback for legacy builds
    if(DEFINED ENV{WOKWI_BUILD})
        set(BSP_SRCS "bsp_wokwi.c" "bsp_i2c.c" "bsp_oled.c")
        set(BSP_REQUIRES driver esp_adc esp_driver_spi esp_driver_i2c spi_flash u8g2)
        message(STATUS "Building for Wokwi (legacy)")
    else()
        set(BSP_SRCS "bsp_heltec_v3.c" "bsp_i2c.c" "bsp_oled.c")
        set(BSP_REQUIRES driver esp_adc esp_driver_spi esp_driver_i2c spi_flash u8g2)
        message(STATUS "Building for Heltec V3 (legacy)")
    endif()
endif()

idf_component_register(
    SRCS ${BSP_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES ${BSP_REQUIRES}
)
