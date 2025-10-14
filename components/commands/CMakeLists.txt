idf_component_register(
    SRCS "commands.c" "xmodem.c"
    INCLUDE_DIRS "include"
    REQUIRES json general_config device_registry lora u8g2 app_update firmware_manifest oled_ui power_mgmt ota_engine uart_commands
)
