#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Hardware
{
    namespace Led
    {
        class LED
        {
        public:
            /**
             * @brief Construct a new LED object
             *
             * @param gpio_pin The GPIO pin number where the LED is connected
             * @param active_low If true, LED is on when pin is LOW (default: true)
             */
            explicit LED(const gpio_num_t gpio_pin, const bool active_low = true);

            /**
             * @brief Destroy the LED object
             *
             * Ensures LED is turned off before destruction
             */
            ~LED();

            // Disable copy operations
            LED(const LED &) = delete;
            LED &operator=(const LED &) = delete;

            // Enable move operations
            LED(LED &&other) noexcept;
            LED &operator=(LED &&other) noexcept;

            /**
             * @brief Turn the LED on
             */
            void On();

            /**
             * @brief Turn the LED off
             */
            void Off();

            /**
             * @brief Toggle the LED state
             */
            void Toggle();

            /**
             * @brief Blink the LED a specified number of times
             *
             * @param count Number of blinks
             * @param interval_ms Interval time between Toggling (milliseconds)
             */
            void Blink(const uint32_t count, const uint32_t interval_ms);

        private:
            static constexpr const char *LOG_TAG = "LED";

            gpio_num_t m_pin_;   ///< GPIO pin number
            bool m_is_on_;       ///< Current LED state
            bool m_active_low_;  ///< LED polarity (true = active low)
            bool m_initialized_; ///< Initialization status

            /**
             * @brief Initialize the GPIO pin for LED control
             */
            void initializeGpio();

            /**
             * @brief Set the physical GPIO level based on polarity
             *
             * @param logical_state The logical state (true = on, false = off)
             */
            void setGpioLevel(const bool logical_state);
        };
    }
}