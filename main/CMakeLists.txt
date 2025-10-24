# Conditional UI system selection
if(CONFIG_UI_EMBEDDED)
    set(UI_COMPONENT "oled_ui")
elseif(CONFIG_UI_RICH)
    set(UI_COMPONENT "ui_rich")
else()
    set(UI_COMPONENT "oled_ui")  # Default fallback
endif()

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp lora usb_hid usb_cdc device_registry ${UI_COMPONENT} button_manager led_manager power_mgmt general_config nvs_flash uart_commands bluetooth_config ota_engine
)
