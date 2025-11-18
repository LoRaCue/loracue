if(NOT CONFIG_UI_RICH)
    idf_component_register(
        SRCS "ui_mini.c"
        INCLUDE_DIRS "include"
        REQUIRES ui_lvgl ui_interface system_events
    )
else()
    # UI_RICH build - provide empty component
    idf_component_register()
endif()
