#include "hcsr04.hpp"

#include "esp_timer.h"
#include "esp_rom_sys.h"

namespace Hardware
{
    namespace Ultrasonic
    {
        HCSR04::HCSR04(gpio_num_t trigger_pin, gpio_num_t echo_pin)
            : trigger_pin_(trigger_pin), echo_pin_(echo_pin)
        {
            // Configure trigger pin as output
            gpio_config_t trigger_config = {
                .pin_bit_mask = (1ULL << trigger_pin_),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE};
            gpio_config(&trigger_config);
            gpio_set_level(trigger_pin_, 0);

            // Configure echo pin as input
            gpio_config_t echo_config = {
                .pin_bit_mask = (1ULL << echo_pin_),
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE};
            gpio_config(&echo_config);
        }

        float HCSR04::MeasureDistance(uint32_t timeout_us)
        {
            // Send 10us trigger pulse
            gpio_set_level(trigger_pin_, 1);
            esp_rom_delay_us(10);
            gpio_set_level(trigger_pin_, 0);

            // Wait for echo to go high
            uint32_t start_wait = esp_timer_get_time();
            while (gpio_get_level(echo_pin_) == 0)
            {
                if ((esp_timer_get_time() - start_wait) > timeout_us)
                {
                    return -1.0f; // Timeout
                }
            }

            // Measure echo pulse width
            uint64_t echo_start = esp_timer_get_time();
            while (gpio_get_level(echo_pin_) == 1)
            {
                if ((esp_timer_get_time() - echo_start) > timeout_us)
                {
                    return -1.0f; // Timeout
                }
            }
            uint64_t echo_end = esp_timer_get_time();

            // Calculate distance (speed of sound: 343 m/s = 0.0343 cm/us)
            // Distance = (time * speed) / 2 (round trip)
            float pulse_duration = (float)(echo_end - echo_start);
            float distance = (pulse_duration * 0.0343f) / 2.0f;

            return distance;
        }
    }
}