idf_component_register(
    SRCS "pca9535.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_i2c bsp
)
