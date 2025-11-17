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
            HCSR04(const gpio_num_t trigger_pin, const gpio_num_t echo_pin);

            /**
             * @brief Measure distance in centimeters:
             * The distance is measured by timing how long the echo pin stays HIGH
             * (the round-trip time of the ultrasonic wave) and converting that time
             * into distance using the speed of sound
             *
             * @param timeout_us Maximum time to wait for echo
             * @return Distance in cm, or -1 on timeout/error
             */
            float MeasureDistance(const uint32_t timeout_us);

        private:
            static constexpr const char *LOG_TAG = "HCSR04";

            gpio_num_t trigger_pin_; ///< GPIO TRIGGER (Output) pin number
            gpio_num_t echo_pin_;    ///< GPIO ECHO (Input) pin number

            /**
             * @brief Configure the Trigger GPIO pin for HCSR04 control
             */
            void configureTriggerGpio();

            /**
             * @brief Configure the Echo GPIO pin for HCSR04 control
             */
            void configureEchoGpio();

            /**
             * @brief Calculate distance (speed of sound: 343 m/s = 0.0343 cm/us)
             *        Distance = (time * speed) / 2 (round trip)
             *
             * @param echo_start Start of the pulse
             * @param echo_end End of the Pulse
             * @return Distance in cm, or -1 on timeout/error
             */
            float calculateDistance(const uint64_t &echo_start, const uint64_t &echo_end);

            /**
             * @brief Set the physical GPIO level of a pin
             *
             * @param gpio_pin The GPIO pin to be set
             * @param level The GPIO level
             */
            void setGpioLevel(const gpio_num_t gpio_pin, const uint32_t level);
        };
    }
}