# Generate fonts from submodule at build time
set(FONT_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/generate_fonts.sh")
set(FONT_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fonts")
set(FONT_SOURCES
    "${FONT_OUTPUT_DIR}/sf_pro_16.c"
    "${FONT_OUTPUT_DIR}/sf_pro_24.c"
    "${FONT_OUTPUT_DIR}/sf_pro_bold_24.c"
)

# Check if fonts need to be generated
if(NOT EXISTS "${FONT_OUTPUT_DIR}/sf_pro_16.c")
    message(STATUS "Generating SF Pro fonts for LVGL...")
    execute_process(
        COMMAND ${FONT_GEN_SCRIPT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE FONT_GEN_RESULT
    )
    if(NOT FONT_GEN_RESULT EQUAL 0)
        message(WARNING "Font generation failed. Using default LVGL fonts.")
        set(FONT_SOURCES "")
    endif()
else()
    message(STATUS "Using existing SF Pro fonts")
endif()

idf_component_register(
    SRCS
        "ui_rich.c"
        "screens/home_screen.c"
        "screens/settings_screen.c"
        "screens/presenter_screen.c"
        "screens/pc_mode_screen.c"
        ${FONT_SOURCES}
    INCLUDE_DIRS
        "include"
        "fonts"
    REQUIRES
        lvgl
        bsp
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
