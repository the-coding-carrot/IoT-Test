#pragma once

#include <cstdint>

#include "cJSON.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "../../config/config.hpp"
#include "../../processor/distance/distance_processor.hpp"

namespace Telemetry
{
    namespace Distance
    {
        class DistanceTelemetry
        {
        public:
            /**
             * @brief Construct a new Distance Telemetry publisher
             *
             * Initializes timing state and logs configuration.
             */
            DistanceTelemetry();

            /**
             * @brief Publish telemetry based on processed distance data
             *
             * Determines what telemetry to emit:
             * - If mail detected: Immediately publish mail_drop event
             * - If mail collected: Immediately publish mail_collected event
             * - If periodic interval elapsed: Publish status telemetry with current state
             *
             * @param data Processed distance data from DistanceProcessor
             * @param baseline_cm Current baseline distance (for context)
             * @param threshold_cm Current trigger threshold (for context)
             *
             * @note Can publish multiple telemetry messages in same call
             * @note Non-blocking: Returns immediately after logging
             */
            void Publish(const Processor::Distance::DistanceData &data,
                         const float baseline_cm, const float threshold_cm);

        private:
            static constexpr const char *LOG_TAG = "DIST_TELE";

            uint64_t last_telemetry_us_ = 0; ///< Timestamp of last periodic telemetry emission (microseconds)

            /**
             * @brief Emit mail drop event telemetry immediately
             *
             * Constructs JSON payload with:
             * - Event type: "mail_drop"
             * - Before/after distance measurements
             * - Delta and duration of occlusion
             * - Computed confidence score
             * - Current success rate
             * - New mailbox state (HAS_MAIL or FULL)
             *
             * @param data Processed distance data containing detection results
             * @param baseline_cm Baseline distance for before/after comparison
             *
             * @note Always publishes when called (not rate-limited)
             */
            void emitMailDropEvent(const Processor::Distance::DistanceData &data, const float &baseline_cm);

            /**
             * @brief Emit mail collected event telemetry immediately
             *
             * Constructs JSON payload with:
             * - Event type: "mail_collected"
             * - Before/after distance measurements
             * - Delta and duration of collection
             * - Current success rate
             * - New mailbox state (EMPTIED)
             *
             * @param data Processed distance data containing collection results
             * @param baseline_cm Baseline distance for reference
             *
             * @note Always publishes when called (not rate-limited)
             */
            void emitMailCollectedEvent(const Processor::Distance::DistanceData &data, const float &baseline_cm);

            /**
             * @brief Conditionally emit periodic status telemetry
             *
             * Publishes current system state at regular intervals for monitoring:
             * - Raw and filtered distance readings
             * - Baseline and threshold references
             * - Measurement success rate
             * - Current mailbox state (empty/has_mail/full/emptied)
             *
             * @param data Current processed distance data
             * @param baseline_cm Current baseline distance
             * @param threshold_cm Current trigger threshold
             *
             * @note Only publishes if TELEMETRY_PERIOD_MS has elapsed since last emission
             * @note Updates last_telemetry_us_ timestamp when published
             */
            void maybeEmitPeriodic(const Processor::Distance::DistanceData &data,
                                   const float &baseline_cm, const float &threshold_cm);

            /**
             * @brief Calculate confidence score for mail drop detection
             *
             * Combines multiple factors into a single confidence metric [0.0, 1.0]:
             * - 50% weight: Distance delta relative to trigger threshold
             * - 30% weight: Occlusion duration relative to hold time
             * - 20% weight: Recent measurement success rate
             *
             * @param data Processed distance data with detection information
             * @return Confidence score between 0.0 (low confidence) and 1.0 (high confidence)
             */
            float calculateConfidence(const Processor::Distance::DistanceData &data) const;

            /**
             * @brief Convert MailboxState enum to string representation
             *
             * @param state MailboxState enum value
             * @return String representation: "empty", "has_mail", "full", "emptied", or "unknown"
             */
            const char *stateToString(const Processor::Distance::MailboxState state) const;

            /**
             * @brief Publish JSON object to output stream
             *
             * Serializes JSON to compact string format and logs via ESP_LOGI.
             * Properly cleans up allocated memory.
             *
             * @param root cJSON object to publish (ownership transferred, will be deleted)
             */
            void publishJSON(cJSON *root);
        };
    }
}