#pragma once

#include "mqtt_client.h"
#include "cJSON.h"

#include <atomic>

namespace Telemetry
{
    namespace MQTT
    {
        class MQTTPublisher
        {
        public:
            /**
             * @brief Construct a new MQTT publisher
             */
            MQTTPublisher();

            /**
             * @brief Destroy the MQTT object
             *
             * Ensures MQTT client is destroyed
             */
            ~MQTTPublisher();

            /**
             * @brief Initialize MQTT client with broker configuration
             *
             * @param broker_uri Full URI of the MQTT broker (e.g., "mqtt://broker.hivemq.com:1883" or "mqtts://..." for TLS)
             * @param client_id Unique client identifier (optional, will be auto-generated if NULL)
             * @param username Username for broker authentication (optional, set to NULL if not required)
             * @param password Password for broker authentication (optional, set to NULL if not required)
             * @return esp_err_t ESP_OK on success, error code otherwise
             */
            esp_err_t Init(const char *broker_uri,
                           const char *client_id = nullptr,
                           const char *username = nullptr,
                           const char *password = nullptr);

            /**
             * @brief Start MQTT client and initiate connection to broker
             *
             * @return esp_err_t ESP_OK on success, error code otherwise
             */
            esp_err_t Start();

            /**
             * @brief Stop MQTT client and disconnect from broker
             *
             * @return esp_err_t ESP_OK on success, error code otherwise
             */
            esp_err_t Stop();

            /**
             * @brief Publish JSON string to specified MQTT topic
             *
             * @param topic MQTT topic to publish to (e.g., "sensors/temperature")
             * @param json JSON-formatted string to publish
             * @param qos Quality of Service level (0, 1, or 2). Default is 1 (at least once delivery)
             * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_STATE if not connected, error code otherwise
             */
            esp_err_t Publish(const char *topic, const char *json, int qos = 1);

            /**
             * @brief Check if MQTT client is currently connected to broker
             *
             * @return true if connected, false otherwise
             */
            bool IsConnected() const { return connected_; }

        private:
            static constexpr const char *LOG_TAG = "MQTT";

            esp_mqtt_client_handle_t client_; ///< Handle to ESP-IDF MQTT client
            std::atomic<bool> connected_;     ///< Connection status flag

            /**
             * @brief Static event handler callback for MQTT events
             *
             * @param handler_args Pointer to MQTTPublisher instance
             * @param base Event base (unused)
             * @param event_id Event ID (unused, handled by MQTT event structure)
             * @param event_data Pointer to esp_mqtt_event_t structure
             */
            static void mqttEventHandler(void *handler_args, esp_event_base_t base,
                                         int32_t event_id, void *event_data);

            /**
             * @brief Handle MQTT events (connected, disconnected, published, error)
             *
             * @param event MQTT event handle containing event type and data
             */
            void handleEvent(esp_mqtt_event_handle_t event);
        };
    }
}