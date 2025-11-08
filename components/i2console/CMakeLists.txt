idf_component_register(
    SRCS "i2console.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp esp_driver_i2c
)
