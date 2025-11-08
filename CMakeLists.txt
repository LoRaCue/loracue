cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2)
set(PICO_PLATFORM rp2350-arm-s)

include(pico-sdk/pico_sdk_init.cmake)

project(I2Console C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Get git version
execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT GIT_VERSION)
    set(GIT_VERSION "unknown")
endif()

# Generate version.h
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
    @ONLY
)

pico_sdk_init()

add_executable(I2Console
    src/main.c
    src/i2c_slave.c
    src/circular_buffer.c
    src/usb_cdc.c
    src/usb_descriptors.c
    src/flash_config.c
    src/lcd_driver.c
    src/lcd_ui.c
    src/log.c
    src/font16.c
)

target_include_directories(I2Console PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(I2Console
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_pwm
    hardware_flash
    hardware_sync
    hardware_watchdog
    pico_unique_id
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_usb(I2Console 0)
pico_enable_stdio_uart(I2Console 0)

pico_add_extra_outputs(I2Console)
