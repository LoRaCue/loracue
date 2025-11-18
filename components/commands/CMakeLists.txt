idf_component_register(
    SRCS "commands.c"
    INCLUDE_DIRS "include"
    REQUIRES json general_config device_registry lora u8g2 app_update power_mgmt ota_engine uart_commands ble system_events esp_tinyusb
)
