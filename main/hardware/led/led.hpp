#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Hardware
{
    namespace Led
    {
        /**
         * @brief LED controller class for GPIO-based LEDs
         */
        class LED
        {
        public:
            /**
             * @brief Construct a new LED object
             *
             * @param gpio_pin The GPIO pin number where the LED is connected
             * @param active_low If true, LED is on when pin is LOW (default: true)
             */
            explicit LED(gpio_num_t gpio_pin, bool active_low = true);

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
             * @param interval_ms Interval time between blinks (milliseconds)
             */
            void Blink(uint32_t count, uint32_t interval_ms);

            /**
             * @brief Set the LED to a specific state
             *
             * @param state true for on, false for off
             */
            void SetState(bool state);

            /**
             * @brief Get the current LED state
             *
             * @return true if LED is on, false if off
             */
            bool GetState() const;

            /**
             * @brief Get the GPIO pin number
             *
             * @return gpio_num_t The GPIO pin this LED is connected to
             */
            gpio_num_t GetPin() const;

        private:
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
            void setGpioLevel(bool logical_state);
        };
    }
}