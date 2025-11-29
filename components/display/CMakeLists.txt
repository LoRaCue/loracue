set(DISPLAY_SRCS "display.c")
set(DISPLAY_REQUIRES bsp esp_lcd lvgl)

if(DEFINED ENV{WOKWI_BUILD})
    list(APPEND DISPLAY_SRCS "display_ssd1306.c")
elseif(CONFIG_BOARD_LILYGO_T5 OR CONFIG_BOARD_LILYGO_T3)
    list(APPEND DISPLAY_SRCS "display_ssd1681.c")
    list(APPEND DISPLAY_REQUIRES espressif__esp_lcd_ssd1681)
elseif(CONFIG_BOARD_HELTEC_V3)
    list(APPEND DISPLAY_SRCS "display_ssd1306.c")
else()
    # Default
    list(APPEND DISPLAY_SRCS "display_ssd1306.c")
endif()

idf_component_register(
    SRCS ${DISPLAY_SRCS}
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        ${DISPLAY_REQUIRES}
)
