#pragma once

#include "mqtt_client.h"
#include "cJSON.h"

#include <atomic>

namespace Telemetry
{
    namespace Publisher
    {
        class MQTTPublisher
        {
        public:
            // Construct a new MQTT publisher
            MQTTPublisher();

            // Destroy the MQTT object
            ~MQTTPublisher();

            // Initialize MQTT client with broker configuration
            esp_err_t Init(const char *broker_uri,
                           const char *client_id = nullptr,
                           const char *username = nullptr,
                           const char *password = nullptr);

            // Start MQTT client and initiate connection to broker
            esp_err_t Start();

            // Stop MQTT client and disconnect from broker
            esp_err_t Stop();

            // Publish JSON string to specified MQTT topic
            esp_err_t Publish(const char *topic, const char *json, int qos = 1);

            // Check if MQTT client is currently connected to broker
            bool IsConnected() const { return connected_; }

        private:
            static constexpr const char *LOG_TAG = "PUBLISHER";

            esp_mqtt_client_handle_t client_; ///< Handle to ESP-IDF MQTT client
            std::atomic<bool> connected_;     ///< Connection status flag

            // Static event handler callback for MQTT events
            static void mqttEventHandler(void *handler_args, esp_event_base_t base,
                                         int32_t event_id, void *event_data);

            // Handle MQTT events (connected, disconnected, published, error)
            void handleEvent(esp_mqtt_event_handle_t event);
        };
    }
}