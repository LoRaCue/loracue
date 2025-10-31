idf_component_register(
    SRCS "tps65185.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_i2c
)
