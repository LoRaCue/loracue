idf_component_register(
    SRCS "config_wifi_server.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif esp_http_server nvs_flash esp_event lwip spiffs json app_update device_registry device_config lora
)
