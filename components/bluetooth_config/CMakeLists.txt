idf_component_register(
    SRCS "bluetooth_config.c" "ble_ota.c"
    INCLUDE_DIRS "include"
    REQUIRES bt commands general_config nvs_flash ota_engine
)
