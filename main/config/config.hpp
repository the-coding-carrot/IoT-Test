#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

namespace Config
{
    // GPIO pin for the onboard LED
    constexpr gpio_num_t LED_PIN = GPIO_NUM_8;

    // LED polarity (true = active low, false = active high)
    constexpr bool LED_ACTIVE_LOW = true;

    // Number of startup blinks (visual indicator)
    constexpr uint32_t STARTUP_BLINK_COUNT = 5;

    // Startup blink timing (milliseconds)
    constexpr uint32_t STARTUP_BLINK_MS = 1000;

    // Application name for logging
    constexpr const char *APP_NAME = "IoT Test";

    // Application version
    constexpr const char *APP_VERSION = "1.0.0";

    // HC-SR04 Ultrasonic Sensor pins
    constexpr gpio_num_t HCSR04_TRIGGER_PIN = GPIO_NUM_2;
    constexpr gpio_num_t HCSR04_ECHO_PIN = GPIO_NUM_3;

    // Measurement settings
    constexpr uint32_t DISTANCE_MEASUREMENT_INTERVAL_MS = 1000;
    constexpr uint32_t TRIGGER_PULSE_uS = 10;
    constexpr float DISTANCE_THRESHOLD_CM = 400.0f; // Alert threshold
    constexpr uint32_t ECHO_TIMEOUT_MS = 35000;     // Maximum time to wait for echo
}