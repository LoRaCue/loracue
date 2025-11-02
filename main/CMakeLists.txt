if(CONFIG_UI_RICH)
    set(UI_IMPL ui_rich)
else()
    set(UI_IMPL ui_mini)
endif()

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp lora usb_hid usb_cdc device_registry ui_interface system_events common_types ${UI_IMPL}
             button_manager led_manager power_mgmt general_config 
             nvs_flash uart_commands bluetooth_config ota_engine
             pc_mode_manager presenter_mode_manager
)
