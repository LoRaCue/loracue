idf_component_register(
    SRCS "usb_cdc.c"
    INCLUDE_DIRS "include"
    REQUIRES "json" "device_config" "device_registry" "lora" "espressif__esp_tinyusb" "app_update"
)
