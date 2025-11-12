#include "config/config.hpp"
#include "hardware/led/led.hpp"
#include "hardware/ultrasonic/hcsr04.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *LOG_TAG = "APP";

extern "C" void app_main(void)
{
    ESP_LOGI(LOG_TAG, "%s v%s", Config::APP_NAME, Config::APP_VERSION);
    ESP_LOGI(LOG_TAG, "Compiled: %s %s", __DATE__, __TIME__);

    // Initialize LED
    Hardware::Led::LED led(Config::LED_PIN, Config::LED_ACTIVE_LOW);
    ESP_LOGI(LOG_TAG, "Performing startup blink sequence...");
    led.Blink(Config::STARTUP_BLINK_COUNT, Config::STARTUP_BLINK_MS);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Initialize HC-SR04
    Hardware::Ultrasonic::HCSR04 ultrasonic(
        Config::HCSR04_TRIGGER_PIN,
        Config::HCSR04_ECHO_PIN);
    ESP_LOGI(LOG_TAG, "HC-SR04 initialized");

    // Main loop
    while (true)
    {
        float distance = ultrasonic.MeasureDistance(/*timeout_us=*/35000);

        if (distance > 0)
        {
            ESP_LOGI(LOG_TAG, "Distance: %.2f cm", distance);

            if (distance < Config::DISTANCE_THRESHOLD_CM)
            {
                led.On();
            }
            else
            {
                led.Off();
            }
        }
        else
        {
            ESP_LOGI(LOG_TAG, "Distance: %.2f cm", distance);
            ESP_LOGW(LOG_TAG, "Measurement timeout or error");
        }

        vTaskDelay(pdMS_TO_TICKS(Config::DISTANCE_MEASUREMENT_INTERVAL_MS));
    }
}