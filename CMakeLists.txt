cmake_minimum_required(VERSION 3.16)

# Set the target before project()
set(IDF_TARGET esp32s3)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(loracue)
