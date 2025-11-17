# Source files
set(COMPONENT_SRCS
    "main.cpp"
    "hardware/led/led.cpp"
    "hardware/ultrasonic/hcsr04.cpp"
    "telemetry/distance/distance_telemetry.cpp"
    "processor/distance/distance_processor.cpp"
)

# Public include directories
set(COMPONENT_INCLUDE_DIRS
    "."
    "hardware/led"
    "hardware/ultrasonic"
    "telemetry/distance"
    "processor/distance"
    "config"
)

# Register component with ESP-IDF
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES 
        driver
        esp_timer
        cjson
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-function
    -fno-exceptions 
    -fno-rtti 
    -fmerge-all-constants  
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${COMPONENT_LIB} PRIVATE -g0)
endif()