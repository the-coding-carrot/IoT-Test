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

        struct StateContext
        {
            std::array<float, Config::FILTER_WINDOW> window;
            size_t w_idx;
            size_t w_count;
            float filtered_cm;

            uint32_t ok_count;
            uint32_t total_count;
            uint32_t ms_since_decay;
            float success_rate;
            uint64_t last_update_us;

            MailboxState current_state;
            bool occluding;
            uint64_t occlusion_start_us;
            uint64_t state_change_us;
            uint64_t refractory_until_us;
        };

        class DistanceProcessor
        {
        public:
            // Construct a new Distance Processor (First Boot)
            DistanceProcessor();

            // Restore constructor (Wake from sleep)
            explicit DistanceProcessor(const StateContext &ctx);

            /**
             * Process a raw distance measurement through the complete pipeline
             *
             * Processing stages:
             * 1. Record measurement success/failure for quality metrics
             * 2. Add measurement to median filter window
             * 3. Update success rate periodically with decay
             * 4. Run state machine to detect mail drops and collections
             * 5. Return consolidated results
             */
            DistanceData Process(const float raw_distance_cm, const uint64_t current_time_us);

            // Helper to extract state for saving to RTC
            StateContext GetContext() const;

            // Get the configured baseline distance
            float GetBaseline() const;

            // Get the computed trigger threshold distance
            float GetThreshold() const;

            // Get the computed full mailbox threshold distance
            float GetFullThreshold() const;

            // Check if detector is currently in refractory period
            bool InRefractory(const uint64_t current_time_us) const;

            // Get the current mailbox state
            MailboxState GetState() const;

        private:
            static constexpr const char *LOG_TAG = "DIST_PROC";

            StateContext ctx_;

            // Detection
            const float baseline_cm_;       ///< Configured baseline distance (empty mailbox) in centimeters
            const float trigger_thresh_cm_; ///< Computed trigger threshold (baseline - delta) in centimeters
            const float full_thresh_cm_;    ///< Threshold for considering mailbox full (baseline - 2*delta) in centimeters
            const float empty_thresh_cm_;   ///< Threshold for considering mailbox empty (baseline - delta/2) in centimeters

            // Add a measurement to the median filter window
            void addToFilter(const float &distance_cm);

            /**
             * Calculate median of valid samples in the filter window
             *
             * Algorithm:
             * 1. Extract all positive (valid) samples from window
             * 2. Sort samples in ascending order
             * 3. Return middle value (or average of two middle values for even count)
             */
            float calculateMedian() const;

            // Update success rate and apply exponential decay to counters
            void updateSuccessRate(const uint32_t &elapsed_ms);

            /**
             * Run the state machine to detect mail drops and collections
             *
             * State transitions:
             * - EMPTY -> HAS_MAIL: When sustained occlusion detected (new mail)
             * - HAS_MAIL -> FULL: When distance drops significantly further
             * - HAS_MAIL/FULL -> EMPTIED: When distance returns near baseline
             * - EMPTIED -> EMPTY: After brief hold period
             */
            void updateStateMachine(DistanceData &data, const uint64_t now_us);
        };
    }
}