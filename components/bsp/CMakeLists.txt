# Conditional BSP compilation based on Kconfig selection

if(DEFINED ENV{WOKWI_BUILD})
    set(BSP_SRCS "bsp_wokwi.c" "bsp_i2c.c")
    message(STATUS "Building BSP for Wokwi Simulator (UI_COMPACT)")
elseif(CONFIG_BOARD_LILYGO_T5)
    set(BSP_SRCS "bsp_lilygo_t5.c")
    message(STATUS "Building BSP for LilyGO T5 4.7\" E-Paper (UI_RICH)")
elseif(CONFIG_BOARD_LILYGO_T3)
    set(BSP_SRCS "bsp_lilygo_t3.c")
    message(STATUS "Building BSP for LilyGO T3-S3 2.13\" E-Paper (UI_MINI)")
elseif(CONFIG_BOARD_HELTEC_V3)
    set(BSP_SRCS "bsp_heltec_v3.c" "bsp_i2c.c")
    message(STATUS "Building BSP for Heltec V3 (UI_COMPACT)")
else()
    # Default fallback (useful for initial config)
    set(BSP_SRCS "bsp_heltec_v3.c" "bsp_i2c.c")
    message(STATUS "Building BSP for Heltec V3 (default)")
endif()

# Include all possible requirements - unused ones won't be linked
idf_component_register(
    SRCS ${BSP_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc esp_driver_spi esp_driver_i2c spi_flash esp_timer efuse
             lvgl pca9535 tps65185 gt911 bq25896 bq27220 pcf85063 display
)
