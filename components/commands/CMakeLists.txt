idf_component_register(
    SRCS "commands.c"
    INCLUDE_DIRS "include"
    REQUIRES json device_config device_registry lora u8g2 app_update firmware_manifest
)
