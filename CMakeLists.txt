cmake_minimum_required(VERSION 3.16)

# Set the target before project()
set(IDF_TARGET esp32s3)

# Generate version header from GitVersion
set(VERSION_HEADER_PATH "${CMAKE_BINARY_DIR}/include/version.h")
execute_process(
    COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/generate_version.py" "${VERSION_HEADER_PATH}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE VERSION_RESULT
)

if(NOT VERSION_RESULT EQUAL 0)
    message(WARNING "Failed to generate version header, using fallback")
endif()

# Add version header to global include path
include_directories("${CMAKE_BINARY_DIR}/include")

# Set project name based on board (used as board_id in app descriptor)
if(DEFINED ENV{SIMULATOR_BUILD})
    set(PROJECT_NAME "wokwi_sim")
else()
    set(PROJECT_NAME "heltec_v3")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})
