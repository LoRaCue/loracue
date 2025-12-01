idf_component_register(
    SRCS "input_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer freertos bsp led_manager ui_lvgl power_mgmt system_events common_types
)
