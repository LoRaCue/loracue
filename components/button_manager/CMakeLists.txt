idf_component_register(
    SRCS "button_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp oled_ui power_mgmt freertos
)
