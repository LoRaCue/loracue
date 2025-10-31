idf_component_register(
    SRCS "button_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp ui_mini led_manager power_mgmt freertos
)
