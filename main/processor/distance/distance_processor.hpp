#pragma once

#include <cstdint>
#include <array>
#include <algorithm>

#include "esp_timer.h"
#include "esp_log.h"

#include "../../config/config.hpp"

namespace Processor
{
    namespace Distance
    {
        enum class MailboxState
        {
            EMPTY,    ///< Mailbox is empty (distance near baseline)
            HAS_MAIL, ///< Mailbox contains mail (distance below threshold, stable)
            FULL,     ///< Mailbox is full (distance significantly below threshold)
            EMPTIED   ///< Mailbox was just emptied (transitional state)
        };

        struct DistanceData
        {
            float raw_cm;         ///< Raw distance measurement from sensor in centimeters (negative if invalid/timeout)
            float filtered_cm;    ///< Median-filtered distance in centimeters (negative if insufficient valid samples)
            float success_rate;   ///< Current measurement success rate (0.0 to 1.0, where 1.0 = 100% success)
            bool mail_detected;   ///< True if a NEW mail drop event was detected during this processing cycle
            bool mail_collected;  ///< True if mail collection (emptying) was detected during this processing cycle
            float delta_cm;       ///< Distance change from baseline that triggered detection (centimeters)
            uint32_t duration_ms; ///< Duration the occlusion was held before triggering event (milliseconds)
            MailboxState state;   ///< Current mailbox state
        };

        class DistanceProcessor
        {
        public:
            /**
             * @brief Construct a new Distance Processor
             *
             * Initializes baseline and threshold values from configuration.
             * Logs initialization parameters for debugging.
             */
            DistanceProcessor();

            /**
             * @brief Process a raw distance measurement through the complete pipeline
             *
             * Processing stages:
             * 1. Record measurement success/failure for quality metrics
             * 2. Add measurement to median filter window
             * 3. Update success rate periodically with decay
             * 4. Run state machine to detect mail drops and collections
             * 5. Return consolidated results
             *
             * @param raw_distance_cm Raw distance from sensor in centimeters
             *                        Negative or zero indicates invalid/timeout measurement
             * @return DistanceData Complete processed results including detection status
             */
            DistanceData Process(const float raw_distance_cm);

            /**
             * @brief Get the configured baseline distance
             * @return Baseline distance in centimeters (empty mailbox reference)
             */
            float GetBaseline() const;

            /**
             * @brief Get the computed trigger threshold distance
             * @return Threshold in centimeters (baseline - trigger_delta)
             */
            float GetThreshold() const;

            /**
             * @brief Get the computed full mailbox threshold distance
             * @return Threshold in centimeters for considering mailbox full
             */
            float GetFullThreshold() const;

            /**
             * @brief Check if detector is currently in refractory period
             * @return true if within refractory cooldown, false otherwise
             *
             * During refractory period, no new mail drop events can be generated.
             * This prevents duplicate events from a single mail delivery.
             */
            bool InRefractory() const;

            /**
             * @brief Get the current mailbox state
             * @return Current MailboxState enum value
             */
            MailboxState GetState() const;

        private:
            static constexpr const char *LOG_TAG = "DIST_PROC";

            // Filtering
            std::array<float, Config::FILTER_WINDOW> window_{}; ///< Circular buffer storing recent distance measurements for filtering
            size_t w_idx_ = 0;                                  ///< Current write index in circular buffer (0 to FILTER_WINDOW-1)
            size_t w_count_ = 0;                                ///< Number of valid samples currently in buffer (0 to FILTER_WINDOW)
            float filtered_cm_ = -1.0f;                         ///< Most recently computed median-filtered distance (centimeters)

            // Success tracking
            uint32_t ok_count_ = 0;       ///< Count of successful measurements (distance > 0)
            uint32_t total_count_ = 0;    ///< Total count of all measurement attempts (success + failure)
            uint32_t ms_since_decay_ = 0; ///< Accumulated milliseconds since last counter decay
            float success_rate_ = 0.0f;   ///< Computed success rate (ok_count / total_count), range [0.0, 1.0]
            uint64_t last_update_us_ = 0; ///< Timestamp of last success rate update (microseconds)

            // Detection
            const float baseline_cm_;       ///< Configured baseline distance (empty mailbox) in centimeters
            const float trigger_thresh_cm_; ///< Computed trigger threshold (baseline - delta) in centimeters
            const float full_thresh_cm_;    ///< Threshold for considering mailbox full (baseline - 2*delta) in centimeters
            const float empty_thresh_cm_;   ///< Threshold for considering mailbox empty (baseline - delta/2) in centimeters

            MailboxState current_state_ = MailboxState::EMPTY; ///< Current mailbox state
            bool occluding_ = false;                           ///< True if currently detecting an occlusion (distance below threshold)
            uint64_t occlusion_start_us_ = 0;                  ///< Timestamp when current occlusion started (microseconds)
            uint64_t state_change_us_ = 0;                     ///< Timestamp of last state change (microseconds)
            uint64_t refractory_until_us_ = 0;                 ///< Timestamp when refractory period ends (microseconds)

            /**
             * @brief Add a measurement to the median filter window
             *
             * Updates the circular buffer and recalculates the median.
             *
             * @param distance_cm Distance measurement to add (negative = invalid)
             */
            void addToFilter(const float &distance_cm);

            /**
             * @brief Calculate median of valid samples in the filter window
             *
             * Algorithm:
             * 1. Extract all positive (valid) samples from window
             * 2. Sort samples in ascending order
             * 3. Return middle value (or average of two middle values for even count)
             *
             * @return Median distance in centimeters, or -1.0 if no valid samples
             *
             * @note Ignores invalid (negative/zero) measurements
             */
            float calculateMedian() const;

            /**
             * @brief Update success rate and apply exponential decay to counters
             *
             * Recomputes success_rate as (ok_count / total_count).
             * Applies decay every ~60 seconds to prevent counter overflow and
             * gradually weight recent measurements more heavily.
             *
             * @param elapsed_ms Milliseconds elapsed since last update
             */
            void updateSuccessRate(const uint32_t &elapsed_ms);

            /**
             * @brief Run the state machine to detect mail drops and collections
             *
             * State transitions:
             * - EMPTY -> HAS_MAIL: When sustained occlusion detected (new mail)
             * - HAS_MAIL -> FULL: When distance drops significantly further
             * - HAS_MAIL/FULL -> EMPTIED: When distance returns near baseline
             * - EMPTIED -> EMPTY: After brief hold period
             *
             * @param data Reference to DistanceData structure to update with events
             * @param now_us Current timestamp in microseconds
             */
            void updateStateMachine(DistanceData &data, const uint64_t now_us);
        };
    }
}