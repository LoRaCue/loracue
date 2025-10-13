idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp lora lora_comm usb_hid usb_cdc device_registry oled_ui button_manager led_manager power_mgmt general_config nvs_flash uart_commands bluetooth_config
)

# Create SPIFFS image from web-interface (uses CONFIG_SPIFFS_OBJ_NAME_LEN=64 from sdkconfig)
# spiffs_create_partition_image(spiffs ../web-interface/out FLASH_IN_PROJECT)
