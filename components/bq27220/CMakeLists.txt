idf_component_register(
    SRCS "bq27220.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_i2c bsp
)
