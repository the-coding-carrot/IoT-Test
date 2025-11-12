set(COMPONENT_SRCS
    "main.cpp"
    "hardware/led/led.cpp"
    "hardware/ultrasonic/hcsr04.cpp"
)

set(COMPONENT_INCLUDE_DIRS
    "."
    "hardware/led"
    "hardware/ultrasonic"
    "config"
)

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    PRIV_INCLUDE_DIRS ${COMPONENT_PRIV_INCLUDEDIRS}

    REQUIRES 
        driver          # GPIO and hardware drivers
        esp_timer
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-function
)