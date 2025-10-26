set(component_srcs "sx126x.c")

idf_component_register(SRCS "${component_srcs}"
                       REQUIRES bsp
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS ".")
