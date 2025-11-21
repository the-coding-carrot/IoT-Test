#include "config/config.hpp"
#include "hardware/led/led.hpp"
#include "hardware/ultrasonic/hcsr04.hpp"
#include "processor/distance/distance_processor.hpp"
#include "telemetry/distance/distance_telemetry.hpp"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include <atomic>
#include <cstring>

static const char *LOG_TAG = "APP";

// Atomic flag to track Wi-Fi status without blocking the main loop
std::atomic<bool> wifi_connected{false};

// Event handler for Wi-Fi and IP events
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_connected = false;
        ESP_LOGW(LOG_TAG, "Wi-Fi disconnected, retrying...");
        // continuous retry
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(LOG_TAG, "Wi-Fi Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(LOG_TAG, "%s v%s", Config::APP_NAME, Config::APP_VERSION);

    // Initialize NVS (REQUIRED for Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Network (Non-blocking)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, Config::CONN_SSID);
    strcpy((char *)wifi_config.sta.password, Config::PASSWORD);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Initialize Hardware
    Hardware::Led::LED led(Config::LED_PIN, Config::LED_ACTIVE_LOW);
    ESP_LOGI(LOG_TAG, "Performing startup blink sequence...");
    led.Blink(Config::STARTUP_BLINK_COUNT, Config::STARTUP_BLINK_MS);

    Hardware::Ultrasonic::HCSR04 ultrasonic(
        Config::HCSR04_TRIGGER_PIN,
        Config::HCSR04_ECHO_PIN);

    Processor::Distance::DistanceProcessor distanceProcessor;
    Telemetry::Distance::DistanceTelemetry distanceTelemetry;

    // Initialize MQTT
    // (Safe to do even if Wi-Fi isn't connected yet; ESP-IDF client handles reconnection)
    esp_err_t err = distanceTelemetry.InitMQTT(
        Config::MQTT_BROKER_URI,
        Config::MQTT_BASE_TOPIC,
        Config::MQTT_CLIENT_ID,
        nullptr,
        nullptr);

    if (err == ESP_OK)
        ESP_LOGI(LOG_TAG, "MQTT client started (background)");
    else
        ESP_LOGE(LOG_TAG, "Failed to start MQTT client");

    ESP_LOGI(LOG_TAG, "Entering measurement loop...");

    while (true)
    {
        float raw_distance = ultrasonic.MeasureDistance(Config::ECHO_TIMEOUT_MS);

        Processor::Distance::DistanceData distanceData = distanceProcessor.Process(raw_distance);

        // Visual Feedback
        if (distanceData.mail_detected)
        {
            led.Blink(10, 100);
        }
        else if (distanceData.mail_collected)
        {
            led.Blink(5, 200);
        }
        else
        {
            // State-based indicators
            switch (distanceData.state)
            {
            case Processor::Distance::MailboxState::EMPTY:
                if (distanceProcessor.InRefractory())
                    led.Blink(2, 300);
                else if (distanceData.success_rate < 0.8f)
                    led.Blink(1, 1000); // Warning: Sensor obscured or failing
                else if (!wifi_connected)
                    led.Blink(1, 50); // Very brief blip if No Wi-Fi
                else
                    led.Off(); // Normal idle state
                break;

            case Processor::Distance::MailboxState::HAS_MAIL:
                led.Blink(1, 500);
                break;

            case Processor::Distance::MailboxState::FULL:
                led.On();
                break;

            case Processor::Distance::MailboxState::EMPTIED:
                led.Blink(3, 150);
                break;
            }
        }

        // Publish Telemetry
        // (Internally checks if MQTT is connected before sending)
        distanceTelemetry.Publish(distanceData,
                                  distanceProcessor.GetBaseline(),
                                  distanceProcessor.GetThreshold());

        vTaskDelay(pdMS_TO_TICKS(Config::DISTANCE_MEASUREMENT_INTERVAL_MS));
    }

    ESP_LOGI(LOG_TAG, "Application terminated");
}