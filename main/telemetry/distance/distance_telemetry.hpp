#pragma once

#include <cstdint>

#include "cJSON.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "../publisher/mqtt.hpp"
#include "../../config/config.hpp"
#include "../../processor/distance/distance_processor.hpp"

namespace Telemetry
{
    namespace Distance
    {
        class DistanceTelemetry
        {
        public:
            // Construct a new Distance Telemetry publisher
            DistanceTelemetry();

            /**
             * Initialize MQTT publishing for distance telemetry
             *
             * Sets up MQTT connection and configures the base topic for all telemetry messages.
             * Messages will be published to subtopics under the base topic:
             * - {base_topic}/events/mail_drop
             * - {base_topic}/events/mail_collected
             * - {base_topic}/status
             */
            esp_err_t InitMQTT(const char *broker_uri,
                               const char *base_topic,
                               const char *client_id = nullptr,
                               const char *username = nullptr,
                               const char *password = nullptr);

            /**
             * Publish telemetry based on processed distance data
             *
             * Determines what telemetry to emit:
             * - If mail detected: Immediately publish mail_drop event
             * - If mail collected: Immediately publish mail_collected event
             * - If periodic interval elapsed: Publish status telemetry with current state
             */
            void Publish(const Processor::Distance::DistanceData &data,
                         const float baseline_cm, const float threshold_cm);

        private:
            static constexpr const char *LOG_TAG = "DTELE";

            uint64_t last_telemetry_us_ = 0;      ///< Timestamp of last periodic telemetry emission (microseconds)
            MQTT::MQTTPublisher *mqtt_publisher_; ///< Pointer to MQTT publisher instance (NULL if not initialized)
            char base_topic_[64];                 ///< Base MQTT topic for all telemetry messages

            /**
             * Emit mail drop event telemetry immediately
             *
             * Constructs JSON payload with:
             * - Event type: "mail_drop"
             * - Before/after distance measurements
             * - Delta and duration of occlusion
             * - Computed confidence score
             * - Current success rate
             * - New mailbox state (HAS_MAIL or FULL)
             */
            void emitMailDropEvent(const Processor::Distance::DistanceData &data, const float &baseline_cm);

            /**
             * Emit mail collected event telemetry immediately
             *
             * Constructs JSON payload with:
             * - Event type: "mail_collected"
             * - Before/after distance measurements
             * - Delta and duration of collection
             * - Current success rate
             * - New mailbox state (EMPTIED)
             */
            void emitMailCollectedEvent(const Processor::Distance::DistanceData &data, const float &baseline_cm);

            /**
             * Conditionally emit periodic status telemetry
             *
             * Publishes current system state at regular intervals for monitoring:
             * - Raw and filtered distance readings
             * - Baseline and threshold references
             * - Measurement success rate
             * - Current mailbox state (empty/has_mail/full/emptied)
             */
            void maybeEmitPeriodic(const Processor::Distance::DistanceData &data,
                                   const float &baseline_cm, const float &threshold_cm);

            /**
             * Calculate confidence score for mail drop detection
             *
             * Combines multiple factors into a single confidence metric [0.0, 1.0]:
             * - 50% weight: Distance delta relative to trigger threshold
             * - 30% weight: Occlusion duration relative to hold time
             * - 20% weight: Recent measurement success rate
             */
            float calculateConfidence(const Processor::Distance::DistanceData &data) const;

            // Convert MailboxState enum to string representation
            const char *stateToString(const Processor::Distance::MailboxState state) const;

            // Publish JSON object via MQTT and log to console
            void publishJSON(cJSON *root, const char *subtopic = "telemetry");
        };
    }
}