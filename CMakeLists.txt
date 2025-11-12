# For detailed information about the ESP-IDF build system, visit:
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html

cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "iot_test")
set(PROJECT_VERSION "1.0.0")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(${PROJECT_NAME})

add_compile_options(
    -Wall                
    -Wextra              
    -Wno-unused-parameter
)

add_compile_definitions(
    PROJECT_VERSION="${PROJECT_VERSION}"
)