#include "config/config.hpp"
#include "hardware/led/led.hpp"
#include "hardware/ultrasonic/hcsr04.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

static const char *LOG_TAG = "APP";

extern "C" void app_main(void)
{
    ESP_LOGI(LOG_TAG, "%s v%s", Config::APP_NAME, Config::APP_VERSION);
    ESP_LOGI(LOG_TAG, "Compiled: %s %s", __DATE__, __TIME__);

    Hardware::Led::LED led(Config::LED_PIN, Config::LED_ACTIVE_LOW);
    ESP_LOGI(LOG_TAG, "Performing startup blink sequence...");
    led.Blink(Config::STARTUP_BLINK_COUNT, Config::STARTUP_BLINK_MS);
    vTaskDelay(pdMS_TO_TICKS(500));

    Hardware::Ultrasonic::HCSR04 ultrasonic(
        Config::HCSR04_TRIGGER_PIN,
        Config::HCSR04_ECHO_PIN);

    ESP_LOGI(LOG_TAG, "Entering measurement loop...");
    while (true)
    {
        float distance = ultrasonic.MeasureDistance(Config::ECHO_TIMEOUT_MS);
        if (distance > 0 && distance < Config::DISTANCE_THRESHOLD_CM)
            led.On();
        else
            led.Off();

        vTaskDelay(pdMS_TO_TICKS(Config::DISTANCE_MEASUREMENT_INTERVAL_MS));
    }

    led.Off();
    ESP_LOGI(LOG_TAG, "Application terminated");
}