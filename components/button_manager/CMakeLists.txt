idf_component_register(
    SRCS "button_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp oled_ui led_manager power_mgmt freertos
)
