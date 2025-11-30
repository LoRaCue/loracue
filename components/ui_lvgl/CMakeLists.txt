file(GLOB FONT_SOURCES "generated/lv_font_*.c")

set(INCLUDE_DIRS "include")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated")
    list(APPEND INCLUDE_DIRS "generated")
endif()

idf_component_register(
    SRCS 
        "ui_lvgl.c"
        "lv_port_disp.c"
        "lv_port_indev.c"
        ${FONT_SOURCES}
    INCLUDE_DIRS 
        ${INCLUDE_DIRS}
    REQUIRES 
        bsp
        display
        lvgl__lvgl
        esp_timer
        espressif__esp_lvgl_port
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
