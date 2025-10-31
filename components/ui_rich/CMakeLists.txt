if(CONFIG_UI_RICH)
    idf_component_register(
        SRCS 
            "ui_rich.c"
            "images/bootscreen.c"
        INCLUDE_DIRS 
            "include"
            "images"
        REQUIRES 
            lvgl
            bsp
            power_mgmt
    )
else()
    # OLED UI build - provide empty component
    idf_component_register()
endif()
