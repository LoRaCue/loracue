idf_component_register(
    SRCS "main.c" "lora_driver.c" "display_ui.c"
    INCLUDE_DIRS "."
    REQUIRES bsp
)
