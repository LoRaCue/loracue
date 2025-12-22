idf_component_register(
    SRCS "ble.c" "ble_ota_handler.c"
    INCLUDE_DIRS "include"
    REQUIRES commands config_manager nvs_flash ota_engine bsp bt ble_ota app_update
)
