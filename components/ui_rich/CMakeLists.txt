if(CONFIG_UI_RICH)
    idf_component_register(
        SRCS 
            "ui_rich.c"
            "images/bootscreen.c"
        INCLUDE_DIRS 
            "include"
            "images"
        REQUIRES 
            bsp
            power_mgmt
            esp_timer
        PRIV_REQUIRES
            lvgl__lvgl
    )
    
    # Tell LVGL to use simple include path
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
    
    # Manually add LVGL include directories for private sources
    idf_component_get_property(lvgl_dir lvgl__lvgl COMPONENT_DIR)
    target_include_directories(${COMPONENT_LIB} PRIVATE 
        "${lvgl_dir}"
        "${lvgl_dir}/src"
    )
else()
    # OLED UI build - provide empty component
    idf_component_register()
endif()
