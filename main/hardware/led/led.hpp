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
            // Construct a new LED object
            explicit LED(const gpio_num_t gpio_pin, const bool active_low = true);

            // Destroy the LED object
            ~LED();

            // Disable copy operations
            LED(const LED &) = delete;
            LED &operator=(const LED &) = delete;

            // Enable move operations
            LED(LED &&other) noexcept;
            LED &operator=(LED &&other) noexcept;

            // Turn the LED on
            void On();

            // Turn the LED off
            void Off();

            // Toggle the LED state
            void Toggle();

            // Blink the LED a specified number of times
            void Blink(const uint32_t count, const uint32_t interval_ms);

        private:
            static constexpr const char *LOG_TAG = "LED";

            gpio_num_t m_pin_;   ///< GPIO pin number
            bool m_is_on_;       ///< Current LED state
            bool m_active_low_;  ///< LED polarity (true = active low)
            bool m_initialized_; ///< Initialization status

            // Initialize the GPIO pin for LED control
            void initializeGpio();

            // Set the physical GPIO level based on polarity
            void setGpioLevel(const bool logical_state);
        };
    }
}