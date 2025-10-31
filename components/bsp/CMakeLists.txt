# Conditional BSP compilation based on board selection
if(NOT BOARD_ID OR BOARD_ID STREQUAL "")
    set(BOARD_ID "heltec_v3")
endif()

if(BOARD_ID STREQUAL "lilygo_t5")
    set(BSP_SRCS "bsp_lilygo_t5.c")
    message(STATUS "Building BSP for LilyGO T5 4.7\" E-Paper (UI_RICH)")
elseif(BOARD_ID STREQUAL "heltec_v3")
    set(BSP_SRCS "bsp_heltec_v3.c" "bsp_i2c.c" "bsp_oled.c")
    message(STATUS "Building BSP for Heltec V3 (UI_MINI)")
elseif(DEFINED ENV{WOKWI_BUILD})
    set(BSP_SRCS "bsp_wokwi.c" "bsp_i2c.c" "bsp_oled.c")
    message(STATUS "Building BSP for Wokwi Simulator (UI_MINI)")
else()
    set(BSP_SRCS "bsp_heltec_v3.c" "bsp_i2c.c" "bsp_oled.c")
    message(STATUS "Building BSP for Heltec V3 (default)")
endif()

# Include all possible requirements - unused ones won't be linked
idf_component_register(
    SRCS ${BSP_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc esp_driver_spi esp_driver_i2c spi_flash esp_timer
             u8g2 lvgl pca9535 tps65185 gt911 bq25896 bq27220 pcf85063
)
