idf_component_register(
    SRCS "presenter_mode_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos lora config_manager common_types input_manager
)
