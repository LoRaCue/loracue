idf_component_register(
    SRCS 
        "commands.c"
        "commands_api.c"
    INCLUDE_DIRS "include"
    REQUIRES json config_manager device_registry lora app_update power_mgmt ota_engine uart_commands ble system_events esp_tinyusb ui_lvgl bsp
)
