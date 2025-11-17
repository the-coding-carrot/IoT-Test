#include "led.hpp"

#include "../../config/config.hpp"

#include "esp_log.h"

#include <utility>

namespace Hardware
{
    namespace Led
    {
        LED::LED(const gpio_num_t gpio_pin, const bool active_low)
            : m_pin_(gpio_pin), m_is_on_(false), m_active_low_(active_low), m_initialized_(false)
        {
            ESP_LOGI(LOG_TAG, "Creating LED on GPIO %d (active %s)",
                     m_pin_, m_active_low_ ? "LOW" : "HIGH");
            initializeGpio();
            Off();
        }

        LED::~LED()
        {
            if (m_initialized_)
            {
                Off();
                ESP_LOGI(LOG_TAG, "Destroying LED on GPIO %d", m_pin_);
            }
        }

        LED::LED(LED &&other) noexcept
            : m_pin_(other.m_pin_), m_is_on_(other.m_is_on_), m_active_low_(other.m_active_low_), m_initialized_(other.m_initialized_)
        {
            other.m_initialized_ = false;
        }

        LED &LED::operator=(LED &&other) noexcept
        {
            if (this != &other)
            {
                if (m_initialized_)
                    Off();

                m_pin_ = other.m_pin_;
                m_is_on_ = other.m_is_on_;
                m_active_low_ = other.m_active_low_;
                m_initialized_ = other.m_initialized_;

                other.m_initialized_ = false;
            }
            return *this;
        }

        void LED::On()
        {
            if (!m_initialized_)
            {
                ESP_LOGW(LOG_TAG, "LED not initialized");
                return;
            }

            setGpioLevel(true);
            m_is_on_ = true;
            ESP_LOGD(LOG_TAG, "LED on GPIO %d turned ON", m_pin_);
        }

        void LED::Off()
        {
            if (!m_initialized_)
            {
                ESP_LOGW(LOG_TAG, "LED not initialized");
                return;
            }

            setGpioLevel(false);
            m_is_on_ = false;
            ESP_LOGD(LOG_TAG, "LED on GPIO %d turned OFF", m_pin_);
        }

        void LED::Toggle()
        {
            if (m_is_on_)
                Off();
            else
                On();
        }

        void LED::Blink(const uint32_t count, const uint32_t interval_ms)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                Toggle();

                // Don't delay after the last blink
                if (i < count - 1)
                    vTaskDelay(pdMS_TO_TICKS(interval_ms));
            }
        }

        void LED::initializeGpio()
        {
            esp_err_t err = gpio_reset_pin(m_pin_);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "Failed to reset GPIO %d: %s", m_pin_, esp_err_to_name(err));
                return;
            }

            err = gpio_set_direction(m_pin_, GPIO_MODE_OUTPUT);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "Failed to set GPIO %d direction: %s", m_pin_, esp_err_to_name(err));
                return;
            }

            m_initialized_ = true;
            ESP_LOGI(LOG_TAG, "GPIO %d initialized successfully", m_pin_);
        }

        void LED::setGpioLevel(const bool logical_state)
        {
            // Convert logical state to physical GPIO level based on polarity
            uint32_t gpio_level = (logical_state == m_active_low_) ? 0 : 1;
            esp_err_t err = gpio_set_level(m_pin_, gpio_level);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "Failed to set GPIO %d level: %s", m_pin_, esp_err_to_name(err));
                return;
            }
        }
    }
}