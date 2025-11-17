#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

/**
 * @namespace Config
 * @brief Global configuration constants for the IoT mailbox monitoring system
 */
namespace Config
{
    constexpr const char *APP_NAME = "IoT Test"; ///< Application name for logging
    constexpr const char *APP_VERSION = "1.0.0"; ///< Application version

    constexpr gpio_num_t LED_PIN = GPIO_NUM_8;  ///< GPIO pin number for the onboard status LED
    constexpr bool LED_ACTIVE_LOW = true;       ///< LED polarity (true = active low, false = active high)
    constexpr uint32_t STARTUP_BLINK_COUNT = 5; ///< Number of startup blinks (visual indicator)
    constexpr uint32_t STARTUP_BLINK_MS = 1000; ///< Startup blink timing (milliseconds)

    constexpr gpio_num_t HCSR04_TRIGGER_PIN = GPIO_NUM_2; ///< GPIO pin connected to HC-SR04 TRIGGER (output from ESP32)
    constexpr gpio_num_t HCSR04_ECHO_PIN = GPIO_NUM_3;    ///< GPIO pin connected to HC-SR04 ECHO (input to ESP32)

    constexpr uint32_t DISTANCE_MEASUREMENT_INTERVAL_MS = 1000; ///< Time interval between consecutive distance measurements (milliseconds)
    constexpr uint32_t TRIGGER_PULSE_uS = 10;                   ///< Duration of trigger pulse sent to HC-SR04 (microseconds)
    constexpr float DISTANCE_THRESHOLD_CM = 400.0f;             ///< Maximum valid distance measurement threshold (centimeters)
    constexpr uint32_t ECHO_TIMEOUT_MS = 35000;                 ///< Maximum time to wait for echo pulse response (microseconds)

    constexpr float BASELINE_CM = 40.0f;     ///< Baseline distance representing an empty mailbox (centimeters)
    constexpr float TRIGGER_DELTA_CM = 3.0f; ///< Minimum distance change required to trigger occlusion detection (centimeters)
    constexpr uint32_t HOLD_MS = 250;        ///< Minimum duration an occlusion must persist to generate a mail drop event (milliseconds)
    constexpr uint32_t REFRACTORY_MS = 8000; ///< Refractory period after a detected mail drop (milliseconds)

    constexpr uint8_t FILTER_WINDOW = 5; ///< Number of samples in the median filter sliding window

    constexpr uint32_t TELEMETRY_PERIOD_MS = 10000; ///< Time interval for periodic telemetry broadcasts (milliseconds)
}