#include "distance_telemetry.hpp"

namespace Telemetry
{
    namespace Distance
    {
        DistanceTelemetry::DistanceTelemetry()
        {
            base_topic_[0] = '\0';
            ESP_LOGI(LOG_TAG, "Telemetry initialized.");
        }

        esp_err_t DistanceTelemetry::InitMQTT(const char *broker_uri,
                                              const char *base_topic,
                                              const char *client_id,
                                              const char *username,
                                              const char *password)
        {
            mqtt_publisher_ = new MQTT::MQTTPublisher();
            if (!mqtt_publisher_)
            {
                ESP_LOGE(LOG_TAG, "Failed to allocate MQTT publisher");
                return ESP_ERR_NO_MEM;
            }

            strncpy(base_topic_, base_topic, sizeof(base_topic_) - 1);
            base_topic_[sizeof(base_topic_) - 1] = '\0';

            esp_err_t err = mqtt_publisher_->Init(broker_uri, client_id, username, password);
            if (err != ESP_OK)
            {
                delete mqtt_publisher_;
                mqtt_publisher_ = nullptr;
                return err;
            }

            return mqtt_publisher_->Start();
        }

        void DistanceTelemetry::Publish(const Processor::Distance::DistanceData &data,
                                        const float baseline_cm, const float threshold_cm)
        {
            // Emit event telemetry
            if (data.mail_detected)
                emitMailDropEvent(data, baseline_cm);

            if (data.mail_collected)
                emitMailCollectedEvent(data, baseline_cm);

            // Emit periodic status telemetry
            maybeEmitPeriodic(data, baseline_cm, threshold_cm);
        }

        void DistanceTelemetry::emitMailDropEvent(const Processor::Distance::DistanceData &data, const float &baseline_cm)
        {
            const float confidence = calculateConfidence(data);

            cJSON *root = cJSON_CreateObject();
            if (!root)
                return;

            cJSON_AddStringToObject(root, "event", "mail_drop");
            cJSON_AddNumberToObject(root, "baseline_cm", baseline_cm);
            cJSON_AddNumberToObject(root, "before_cm", baseline_cm);
            cJSON_AddNumberToObject(root, "after_cm", data.filtered_cm);
            cJSON_AddNumberToObject(root, "delta_cm", data.delta_cm);
            cJSON_AddNumberToObject(root, "duration_ms", data.duration_ms);
            cJSON_AddNumberToObject(root, "confidence", confidence);
            cJSON_AddNumberToObject(root, "success_rate", data.success_rate);
            cJSON_AddStringToObject(root, "new_state", stateToString(data.state));

            publishJSON(root, "events/mail_drop");
        }

        void DistanceTelemetry::emitMailCollectedEvent(const Processor::Distance::DistanceData &data, const float &baseline_cm)
        {
            cJSON *root = cJSON_CreateObject();
            if (!root)
                return;

            cJSON_AddStringToObject(root, "event", "mail_collected");
            cJSON_AddNumberToObject(root, "baseline_cm", baseline_cm);
            cJSON_AddNumberToObject(root, "before_cm", data.filtered_cm - data.delta_cm);
            cJSON_AddNumberToObject(root, "after_cm", data.filtered_cm);
            cJSON_AddNumberToObject(root, "delta_cm", data.delta_cm);
            cJSON_AddNumberToObject(root, "duration_ms", data.duration_ms);
            cJSON_AddNumberToObject(root, "success_rate", data.success_rate);
            cJSON_AddStringToObject(root, "new_state", stateToString(data.state));

            publishJSON(root, "events/mail_collected");
        }

        void DistanceTelemetry::maybeEmitPeriodic(const Processor::Distance::DistanceData &data,
                                                  const float &baseline_cm, const float &threshold_cm)
        {
            const uint64_t now_us = esp_timer_get_time();

            cJSON *root = cJSON_CreateObject();
            if (!root)
                return;

            cJSON_AddBoolToObject(root, "telemetry", true);
            cJSON_AddNumberToObject(root, "distance_cm", data.raw_cm);
            cJSON_AddNumberToObject(root, "filtered_cm", data.filtered_cm);
            cJSON_AddNumberToObject(root, "baseline_cm", baseline_cm);
            cJSON_AddNumberToObject(root, "threshold_cm", threshold_cm);
            cJSON_AddNumberToObject(root, "success_rate", data.success_rate);
            cJSON_AddStringToObject(root, "mailbox_state", stateToString(data.state));

            publishJSON(root, "status");
            last_telemetry_us_ = now_us;
        }

        float DistanceTelemetry::calculateConfidence(const Processor::Distance::DistanceData &data) const
        {
            const float delta_component = 0.5f * (data.delta_cm / std::max(0.1f, Config::TRIGGER_DELTA_CM));
            const float duration_component = 0.3f * (static_cast<float>(data.duration_ms) /
                                                     std::max(1.0f, static_cast<float>(Config::HOLD_MS)));
            const float reliability_component = 0.2f * std::clamp(data.success_rate, 0.0f, 1.0f);

            return std::min(1.0f, delta_component + duration_component + reliability_component);
        }

        const char *DistanceTelemetry::stateToString(const Processor::Distance::MailboxState state) const
        {
            switch (state)
            {
            case Processor::Distance::MailboxState::EMPTY:
                return "empty";
            case Processor::Distance::MailboxState::HAS_MAIL:
                return "has_mail";
            case Processor::Distance::MailboxState::FULL:
                return "full";
            case Processor::Distance::MailboxState::EMPTIED:
                return "emptied";
            default:
                return "unknown";
            }
        }

        void DistanceTelemetry::publishJSON(cJSON *root, const char *subtopic)
        {
            char *json = cJSON_PrintUnformatted(root);
            if (json)
            {
                ESP_LOGI(LOG_TAG, "%s", json);

                // Publish via MQTT if connected
                if (mqtt_publisher_ && mqtt_publisher_->IsConnected())
                {
                    char topic[128];
                    snprintf(topic, sizeof(topic), "%s/%s", base_topic_, subtopic);
                    mqtt_publisher_->Publish(topic, json);
                }

                cJSON_free(json);
            }
            cJSON_Delete(root);
        }
    }
}