idf_component_register(
    SRCS "bluetooth_config.c"
    INCLUDE_DIRS "include"
    REQUIRES bt commands general_config nvs_flash
)
