#pragma once

#include "driver/gpio.h"

#include <cstdint>

namespace Hardware
{
    namespace Ultrasonic
    {
        class HCSR04
        {
        public:
            // Construct a new HCSR04 sensor object
            HCSR04(const gpio_num_t trigger_pin, const gpio_num_t echo_pin);

            /**
             * Measure distance in centimeters:
             * The distance is measured by timing how long the echo pin stays HIGH
             * (the round-trip time of the ultrasonic wave) and converting that time
             * into distance using the speed of sound
             */
            float MeasureDistance(const uint32_t timeout_us);

        private:
            static constexpr const char *LOG_TAG = "HCSR04";

            gpio_num_t trigger_pin_; ///< GPIO TRIGGER (Output) pin number
            gpio_num_t echo_pin_;    ///< GPIO ECHO (Input) pin number

            // Configure the Trigger GPIO pin for HCSR04 control
            void configureTriggerGpio();

            // Configure the Echo GPIO pin for HCSR04 control
            void configureEchoGpio();

            /**
             * Calculate distance (speed of sound: 343 m/s = 0.0343 cm/us)
             *        Distance = (time * speed) / 2 (round trip)
             */
            float calculateDistance(const uint64_t &echo_start, const uint64_t &echo_end);

            // Set the physical GPIO level of a pin
            void setGpioLevel(const gpio_num_t gpio_pin, const uint32_t level);
        };
    }
}