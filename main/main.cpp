#include "config/config.hpp"
#include "hardware/laser/vl53lxx-v2.hpp"
#include "processor/processor.hpp"
#include "telemetry/telemetry.hpp"

#include "esp_sleep.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#include <string>
#include <map>
#include <optional>

static const char *LOG_TAG = "MAIN";

// This struct stays alive during deep sleep
struct RtcStore
{
    uint32_t boot_count;
    Processor::StateContext processor_state;
    uint64_t last_telemetry_time_sec;
    uint64_t virtual_time_us;
};
RTC_DATA_ATTR RtcStore rtc_store;

std::pair<bool, std::optional<std::string>> connect_wifi_blocking()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, Config::CONN_SSID);
    strcpy((char *)wifi_config.sta.password, Config::PASSWORD);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(LOG_TAG, "Connecting to Wi-Fi...");
    esp_wifi_connect();

    // Wait for IP (Simple blocking wait)
    int retries = 0;
    while (retries < 100)
    { // Wait up to ~10 seconds
        esp_netif_ip_info_t ip_info;
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif)
        {
            if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
            {
                if (ip_info.ip.addr != 0)
                {
                    esp_ip4_addr_t ip = ip_info.ip;
                    char ip_str[16]; // Enough for "xxx.xxx.xxx.xxx"
                    esp_ip4addr_ntoa(&ip, ip_str, sizeof(ip_str));

                    std::string ipAddress(ip_str);
                    ESP_LOGI(LOG_TAG, "Wi-Fi Connected! IP: %s", ipAddress.c_str());

                    return {true, ipAddress};
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        retries++;
    }

    ESP_LOGW(LOG_TAG, "Wi-Fi connection timeout");
    return {false, std::nullopt};
}

extern "C" void app_main(void)
{
    ESP_LOGI(LOG_TAG, "%s v%s", Config::APP_NAME, Config::APP_VERSION);

    // Record wake time to calculate actual wake duration
    uint64_t wake_time_start = esp_timer_get_time();

    // Determine Wakeup Cause & Update Virtual Clock
    bool is_fresh_boot = (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER);

    if (is_fresh_boot)
    {
        ESP_LOGI(LOG_TAG, "Fresh Boot: Initializing State");
        rtc_store.boot_count = 0;
        Processor::Processor temp;
        rtc_store.processor_state = temp.GetContext();
        rtc_store.last_telemetry_time_sec = 0; // Will force immediate heartbeat
        rtc_store.virtual_time_us = 0;
    }
    else
    {
        rtc_store.boot_count++;
        // Advance virtual clock by the sleep duration
        rtc_store.virtual_time_us += Config::DEEP_SLEEP_US;
        ESP_LOGI(LOG_TAG, "Wakeup #%lu (Virtual Time: %llu s)",
                 rtc_store.boot_count,
                 rtc_store.virtual_time_us / 1000000ULL);
    }

    // Initialize Hardware - VL53L0X laser sensor
    Hardware::Laser::VL53L0X laser(Config::VL53L0X_I2C_PORT,
                                   Config::VL53L0X_SDA_PIN,
                                   Config::VL53L0X_SCL_PIN);

    // Restore Processor from RTC
    Processor::Processor processor(rtc_store.processor_state);

    const float raw_dist = laser.MeasureDistance(Config::VL53L0X_TIMEOUT_MS);

    // Pass the virtual time to the processor
    Processor::DistanceData data = processor.Process(raw_dist, rtc_store.virtual_time_us);

    ESP_LOGI(LOG_TAG, "Dist: %.1f cm | State: %d", data.filtered_cm, (int)data.state);

    // Evaluate if radio must wake up
    const bool crucial_event = data.mail_detected || data.mail_collected;

    // Check for periodic update using virtual time in seconds
    const uint64_t virtual_time_sec = rtc_store.virtual_time_us / 1000000ULL;
    const bool periodic_update = (virtual_time_sec >= (rtc_store.last_telemetry_time_sec + Config::HEARTBEAT_INTERVAL_SEC));

    if (crucial_event || periodic_update)
    {
        ESP_LOGI(LOG_TAG, "Connecting to report event (Event=%d, Periodic=%d)...", crucial_event, periodic_update);

        const auto connection_status = connect_wifi_blocking();
        if (connection_status.first)
        {
            Telemetry::Telemetry telemetry;
            telemetry.InitMQTT(Config::MQTT_BROKER_URI, Config::MQTT_BASE_TOPIC, Config::MQTT_CLIENT_ID, nullptr, nullptr);

            vTaskDelay(pdMS_TO_TICKS(1000));
            telemetry.Publish(data, processor.GetBaseline(), processor.GetThreshold(), connection_status.second);
            vTaskDelay(pdMS_TO_TICKS(1000));

            esp_wifi_disconnect();
            esp_wifi_stop();

            // Update last telemetry time after successful transmission
            if (periodic_update)
                rtc_store.last_telemetry_time_sec = virtual_time_sec;
        }
        else
        {
            ESP_LOGW(LOG_TAG, "WiFi connection failed - telemetry skipped");
        }
    }

    // Save State Back to RTC
    rtc_store.processor_state = processor.GetContext();

    // Calculate actual wake duration and add to virtual time
    const uint64_t wake_duration_us = esp_timer_get_time() - wake_time_start;
    rtc_store.virtual_time_us += wake_duration_us;

    ESP_LOGI(LOG_TAG, "Awake for %llu ms, entering deep sleep for %.1f s",
             wake_duration_us / 1000ULL,
             Config::DEEP_SLEEP_US / 1000000.0);

    esp_sleep_enable_timer_wakeup(Config::DEEP_SLEEP_US);
    esp_deep_sleep_start();
}