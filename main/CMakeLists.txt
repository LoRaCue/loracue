idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp lora usb_hid usb_cdc device_registry ui_interface system_events common_types ui_compact
             input_manager led_manager power_mgmt config_manager 
             nvs_flash uart_commands ble ota_engine
             pc_mode_manager presenter_mode_manager i2console
)
