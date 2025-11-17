#include "config/config.hpp"
#include "hardware/led/led.hpp"
#include "hardware/ultrasonic/hcsr04.hpp"
#include "processor/distance/distance_processor.hpp"
#include "telemetry/distance/distance_telemetry.hpp"

#include "esp_log.h"

static const char *LOG_TAG = "APP";

extern "C" void app_main(void)
{
    ESP_LOGI(LOG_TAG, "%s v%s", Config::APP_NAME, Config::APP_VERSION);
    ESP_LOGI(LOG_TAG, "Compiled: %s %s", __DATE__, __TIME__);

    // Initialize hardware
    Hardware::Led::LED led(Config::LED_PIN, Config::LED_ACTIVE_LOW);
    ESP_LOGI(LOG_TAG, "Performing startup blink sequence...");
    led.Blink(Config::STARTUP_BLINK_COUNT, Config::STARTUP_BLINK_MS);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Initialize hardware components
    Hardware::Ultrasonic::HCSR04 ultrasonic(
        Config::HCSR04_TRIGGER_PIN,
        Config::HCSR04_ECHO_PIN);

    // Initialize processors and telemetry
    Processor::Distance::DistanceProcessor distanceProcessor;
    Telemetry::Distance::DistanceTelemetry distanceTelemetry;

    ESP_LOGI(LOG_TAG, "Entering measurement loop...");

    while (true)
    {
        float raw_distance = ultrasonic.MeasureDistance(Config::ECHO_TIMEOUT_MS);

        Processor::Distance::DistanceData distanceData = distanceProcessor.Process(raw_distance);

        if (distanceData.mail_detected)
        {
            // New mail arrived - celebration blink!
            led.Blink(10, 100);
        }
        else if (distanceData.mail_collected)
        {
            // Mail collected - acknowledgment blink
            led.Blink(5, 200);
        }
        else
        {
            // Normal operation - indicate current state
            switch (distanceData.state)
            {
            case Processor::Distance::MailboxState::EMPTY:
                // Mailbox empty and ready
                if (distanceProcessor.InRefractory())
                    led.Blink(2, 300); // Still in cooldown after collection
                else if (distanceData.success_rate < 0.8f)
                    led.Blink(1, 1000); // Sensor warning
                else
                    led.Off(); // All good, waiting for mail
                break;

            case Processor::Distance::MailboxState::HAS_MAIL:
                // Mailbox has mail - slow blink to indicate
                led.Blink(1, 500);
                break;

            case Processor::Distance::MailboxState::FULL:
                // Mailbox full - solid on
                led.On();
                break;

            case Processor::Distance::MailboxState::EMPTIED:
                // Just emptied - fast blink (transitional state)
                led.Blink(3, 150);
                break;
            }
        }

        distanceTelemetry.Publish(distanceData,
                                  distanceProcessor.GetBaseline(),
                                  distanceProcessor.GetThreshold());

        vTaskDelay(pdMS_TO_TICKS(Config::DISTANCE_MEASUREMENT_INTERVAL_MS));
    }

    ESP_LOGI(LOG_TAG, "Application terminated");
}