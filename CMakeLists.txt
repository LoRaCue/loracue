cmake_minimum_required(VERSION 3.16)

# Set default BOARD_ID if not provided (MUST be before anything else)
if(NOT DEFINED BOARD_ID)
    set(BOARD_ID "heltec_v3")
    message(STATUS "BOARD_ID not set, defaulting to: ${BOARD_ID}")
endif()

# Set the target before project()
set(IDF_TARGET esp32s3)

# Get version from GitVersion
execute_process(
    COMMAND gitversion /showvariable FullSemVer
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GITVERSION_FULLSEMVER
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GITVERSION_RESULT
)

if(GITVERSION_RESULT EQUAL 0)
    set(PROJECT_VER "${GITVERSION_FULLSEMVER}")
    message(STATUS "Version from GitVersion: ${PROJECT_VER}")
else()
    set(PROJECT_VER "0.1.0-dev")
    message(WARNING "GitVersion failed, using fallback version: ${PROJECT_VER}")
endif()

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
if(DEFINED BOARD_ID)
    set(PROJECT_NAME "${BOARD_ID}")
else()
    set(PROJECT_NAME "loracue")  # Default fallback
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})
