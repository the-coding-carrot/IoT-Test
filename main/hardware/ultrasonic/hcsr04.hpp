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
            /**
             * @brief Construct a new HCSR04 sensor object
             * @param trigger_pin GPIO pin connected to trigger
             * @param echo_pin GPIO pin connected to echo
             */
            HCSR04(gpio_num_t trigger_pin, gpio_num_t echo_pin);

            /**
             * @brief Measure distance in centimeters
             * @param timeout_us Maximum time to wait for echo (default 30ms)
             * @return Distance in cm, or -1 on timeout/error
             */
            float MeasureDistance(uint32_t timeout_us = 30000);

        private:
            gpio_num_t trigger_pin_;
            gpio_num_t echo_pin_;
        };
    }
}