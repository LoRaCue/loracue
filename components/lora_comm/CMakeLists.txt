idf_component_register(
    SRCS "lora_comm.c"
    INCLUDE_DIRS "include"
    REQUIRES "lora" "button_manager" "general_config" "usb_hid"
)
