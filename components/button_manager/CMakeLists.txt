idf_component_register(
    SRCS "button_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp system_events led_manager power_mgmt freertos common_types ui_lvgl
)
