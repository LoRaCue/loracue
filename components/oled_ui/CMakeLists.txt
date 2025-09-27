idf_component_register(
    SRCS "oled_ui.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp power_mgmt freertos
)
