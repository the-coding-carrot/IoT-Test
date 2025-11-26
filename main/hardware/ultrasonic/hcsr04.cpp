#include "hcsr04.hpp"

#include "../../config/config.hpp"

#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

namespace Hardware
{
    namespace Ultrasonic
    {
        HCSR04::HCSR04(const gpio_num_t trigger_pin, const gpio_num_t echo_pin)
            : trigger_pin_(trigger_pin), echo_pin_(echo_pin)
        {
            configureTriggerGpio();
            configureEchoGpio();

            ESP_LOGI(LOG_TAG, "HC-SR04 configured");
        }

        float HCSR04::MeasureDistance(const uint32_t timeout_us)
        {
            // Send trigger pulse
            setGpioLevel(trigger_pin_, 1);
            esp_rom_delay_us(Config::TRIGGER_PULSE_uS);
            setGpioLevel(trigger_pin_, 0);

            // Small stabilization delay for sensor to process trigger
            esp_rom_delay_us(2);

            // Wait for echo to go high
            uint64_t start_wait = esp_timer_get_time();
            while (gpio_get_level(echo_pin_) == 0)
            {
                if ((esp_timer_get_time() - start_wait) > timeout_us)
                {
                    ESP_LOGW(LOG_TAG, "Timed out waiting for echo");
                    return -1.0f;
                }
            }

            // Measure echo pulse width
            uint64_t echo_start = esp_timer_get_time();
            while (gpio_get_level(echo_pin_) == 1)
            {
                if ((esp_timer_get_time() - echo_start) > timeout_us)
                {
                    ESP_LOGW(LOG_TAG, "Timed out measuring echo pulse width");
                    return -1.0f;
                }
            }
            uint64_t echo_end = esp_timer_get_time();

            return calculateDistance(echo_start, echo_end);
        }

        float HCSR04::calculateDistance(const uint64_t &echo_start, const uint64_t &echo_end)
        {
            float pulse_duration = (float)(echo_end - echo_start);
            float distance = (pulse_duration * 0.0343f) / 2.0f;

            // Validate reading range (HC-SR04 typical range: 2cm - 400cm)
            if (distance < 2.0f)
            {
                ESP_LOGW(LOG_TAG, "Distance below minimum range: %.2f cm", distance);
                return -1.0f;
            }
            else if (distance >= Config::DISTANCE_THRESHOLD_CM)
            {
                ESP_LOGW(LOG_TAG, "Distance threshold achieved or surpassed: %.2f cm over the threshold",
                         (distance - Config::DISTANCE_THRESHOLD_CM));
            }
            else
            {
                ESP_LOGI(LOG_TAG, "Distance: %.2f cm", distance);
            }

            return distance;
        }

        void HCSR04::configureTriggerGpio()
        {
            gpio_config_t trigger_config = {
                .pin_bit_mask = (1ULL << trigger_pin_),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE};

            esp_err_t err = gpio_config(&trigger_config);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "Failed to configure Trigger GPIO %d: %s", trigger_pin_, esp_err_to_name(err));
                return;
            }

            ESP_LOGI(LOG_TAG, "Trigger GPIO %d configured", trigger_pin_);

            setGpioLevel(trigger_pin_, 0);
        }

        void HCSR04::configureEchoGpio()
        {
            gpio_config_t echo_config = {
                .pin_bit_mask = (1ULL << echo_pin_),
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE};
            esp_err_t err = gpio_config(&echo_config);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "Failed to configure Echo GPIO %d: %s", echo_pin_, esp_err_to_name(err));
                return;
            }

            ESP_LOGI(LOG_TAG, "Echo GPIO %d configured", echo_pin_);
        }

        void HCSR04::setGpioLevel(const gpio_num_t gpio_pin, const uint32_t level)
        {
            esp_err_t err = gpio_set_level(gpio_pin, level);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "Failed to set GPIO %d level: %s", gpio_pin, esp_err_to_name(err));
                return;
            }
        }
    }
}