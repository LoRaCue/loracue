idf_component_register(
    SRCS "bluetooth_config.c" "ble_ota_handler.c"
    INCLUDE_DIRS "include"
    REQUIRES commands general_config nvs_flash ota_engine bsp bt espressif__ble_ota app_update
)
