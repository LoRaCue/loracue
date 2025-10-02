idf_component_register(
    SRCS "lora_driver.c" "lora_protocol.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp mbedtls device_registry esp_wifi esp_netif lwip sx126x_lib
)
