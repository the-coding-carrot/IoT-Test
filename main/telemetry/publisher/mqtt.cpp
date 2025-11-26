#include "mqtt.hpp"

#include "esp_log.h"

namespace Telemetry
{
    namespace MQTT
    {
        MQTTPublisher::MQTTPublisher()
            : client_(nullptr), connected_(false)
        {
        }

        MQTTPublisher::~MQTTPublisher()
        {
            if (client_)
                esp_mqtt_client_destroy(client_);
        }

        esp_err_t MQTTPublisher::Init(const char *broker_uri, const char *client_id,
                                      const char *username, const char *password)
        {
            esp_mqtt_client_config_t mqtt_cfg = {};
            mqtt_cfg.broker.address.uri = broker_uri;

            if (client_id)
                mqtt_cfg.credentials.client_id = client_id;

            if (username)
                mqtt_cfg.credentials.username = username;

            if (password)
                mqtt_cfg.credentials.authentication.password = password;

            // Configure connection parameters
            mqtt_cfg.session.keepalive = 60;                 // Send keepalive ping every 60 seconds
            mqtt_cfg.network.reconnect_timeout_ms = 10000;   // Wait 10s before reconnection attempt
            mqtt_cfg.network.disable_auto_reconnect = false; // Enable automatic reconnection

            client_ = esp_mqtt_client_init(&mqtt_cfg);
            if (!client_)
            {
                ESP_LOGE(LOG_TAG, "Failed to initialize MQTT client");
                return ESP_FAIL;
            }

            esp_err_t err = esp_mqtt_client_register_event(client_,
                                                           MQTT_EVENT_ANY,
                                                           mqttEventHandler,
                                                           this);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "Failed to register MQTT event handler: %s",
                         esp_err_to_name(err));
                return err;
            }

            ESP_LOGI(LOG_TAG, "MQTT client initialized: %s", broker_uri);
            return ESP_OK;
        }

        esp_err_t MQTTPublisher::Start()
        {
            if (!client_)
            {
                ESP_LOGE(LOG_TAG, "MQTT client not initialized");
                return ESP_ERR_INVALID_STATE;
            }

            esp_err_t err = esp_mqtt_client_start(client_);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "Failed to start MQTT client: %s",
                         esp_err_to_name(err));
            }
            return err;
        }

        esp_err_t MQTTPublisher::Stop()
        {
            if (!client_)
                return ESP_ERR_INVALID_STATE;

            return esp_mqtt_client_stop(client_);
        }

        esp_err_t MQTTPublisher::Publish(const char *topic, const char *json, int qos)
        {
            if (!client_ || !connected_)
            {
                ESP_LOGW(LOG_TAG, "Cannot publish: not connected");
                return ESP_ERR_INVALID_STATE;
            }

            // Publish message to MQTT broker
            int msg_id = esp_mqtt_client_publish(client_, topic, json, 0, qos, 0);
            if (msg_id < 0)
            {
                ESP_LOGE(LOG_TAG, "Failed to publish message");
                return ESP_FAIL;
            }

            ESP_LOGD(LOG_TAG, "Published to %s, msg_id=%d", topic, msg_id);
            return ESP_OK;
        }

        void MQTTPublisher::mqttEventHandler(void *handler_args, esp_event_base_t base,
                                             int32_t event_id, void *event_data)
        {
            auto *publisher = static_cast<MQTTPublisher *>(handler_args);
            auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
            publisher->handleEvent(event);
        }

        void MQTTPublisher::handleEvent(esp_mqtt_event_handle_t event)
        {
            switch (event->event_id)
            {
            case MQTT_EVENT_CONNECTED:
                ESP_LOGI(LOG_TAG, "Connected to MQTT broker");
                connected_ = true;
                break;

            case MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(LOG_TAG, "Disconnected from MQTT broker");
                connected_ = false;
                break;

            case MQTT_EVENT_PUBLISHED:
                ESP_LOGD(LOG_TAG, "Message published, msg_id=%d", event->msg_id);
                break;

            case MQTT_EVENT_ERROR:
                ESP_LOGE(LOG_TAG, "MQTT error occurred");
                if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
                {
                    ESP_LOGE(LOG_TAG, "TCP transport error: 0x%x",
                             event->error_handle->esp_tls_last_esp_err);
                }
                break;

            default:
                break;
            }
        }
    }
}