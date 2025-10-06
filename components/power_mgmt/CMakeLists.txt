idf_component_register(
    SRCS "power_mgmt.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_pm driver esp_timer freertos
    PRIV_REQUIRES bsp
)
