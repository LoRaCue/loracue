idf_component_register(
    SRCS "pc_mode_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos lora device_registry usb_hid system_events common_types
)
