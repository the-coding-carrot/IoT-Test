#include "distance_processor.hpp"

namespace Processor
{
    namespace Distance
    {
        DistanceProcessor::DistanceProcessor()
            : baseline_cm_(Config::BASELINE_CM),
              trigger_thresh_cm_(Config::BASELINE_CM - Config::TRIGGER_DELTA_CM),
              full_thresh_cm_(Config::BASELINE_CM - (2.0f * Config::TRIGGER_DELTA_CM)),
              empty_thresh_cm_(Config::BASELINE_CM - (Config::TRIGGER_DELTA_CM * 0.5f))
        {
            ESP_LOGI(LOG_TAG, "DistanceProcessor initialized. baseline=%.2f cm, trigger=%.2f cm, full=%.2f cm, empty=%.2f cm",
                     baseline_cm_, trigger_thresh_cm_, full_thresh_cm_, empty_thresh_cm_);
        }

        DistanceData DistanceProcessor::Process(const float raw_distance_cm)
        {
            DistanceData data = {};
            data.raw_cm = raw_distance_cm;
            data.mail_detected = false;
            data.mail_collected = false;
            data.state = current_state_;

            // Track success rate
            total_count_++;
            if (raw_distance_cm > 0)
                ok_count_++;

            // Filter the measurement
            addToFilter(raw_distance_cm);
            data.filtered_cm = filtered_cm_;

            // Update success rate periodically
            const uint64_t now_us = esp_timer_get_time();
            const uint32_t elapsed_ms = static_cast<uint32_t>((now_us - last_update_us_) / 1000ULL);
            if (elapsed_ms >= 1000) // Update every second
            {
                updateSuccessRate(elapsed_ms);
                last_update_us_ = now_us;
            }
            data.success_rate = success_rate_;

            // Run state machine
            updateStateMachine(data, now_us);

            return data;
        }

        float DistanceProcessor::GetBaseline() const { return baseline_cm_; }
        float DistanceProcessor::GetThreshold() const { return trigger_thresh_cm_; }
        float DistanceProcessor::GetFullThreshold() const { return full_thresh_cm_; }
        bool DistanceProcessor::InRefractory() const { return esp_timer_get_time() < refractory_until_us_; }
        MailboxState DistanceProcessor::GetState() const { return current_state_; }

        void DistanceProcessor::addToFilter(const float &distance_cm)
        {
            window_[w_idx_] = distance_cm;
            w_idx_ = (w_idx_ + 1) % Config::FILTER_WINDOW;
            if (w_count_ < Config::FILTER_WINDOW)
                w_count_++;

            filtered_cm_ = calculateMedian();
        }

        float DistanceProcessor::calculateMedian() const
        {
            float tmp[Config::FILTER_WINDOW];
            size_t n = 0;
            for (size_t i = 0; i < w_count_; ++i)
            {
                const float v = window_[i];
                if (v > 0)
                    tmp[n++] = v;
            }
            if (n == 0)
                return -1.0f;

            std::sort(tmp, tmp + n);
            return (n & 1) ? tmp[n / 2] : 0.5f * (tmp[n / 2 - 1] + tmp[n / 2]);
        }

        void DistanceProcessor::updateSuccessRate(const uint32_t &elapsed_ms)
        {
            success_rate_ = (total_count_ > 0)
                                ? static_cast<float>(ok_count_) / static_cast<float>(total_count_)
                                : 0.0f;

            ms_since_decay_ += elapsed_ms;
            if (ms_since_decay_ >= 60000)
            {
                ok_count_ = ok_count_ / 2;
                total_count_ = total_count_ / 2;
                ms_since_decay_ = 0;
            }
        }

        void DistanceProcessor::updateStateMachine(DistanceData &data, const uint64_t now_us)
        {
            // Invalid reading - maintain current state
            if (filtered_cm_ <= 0)
                return;

            const bool in_refractory = (now_us < refractory_until_us_);
            const uint32_t time_in_state_ms = static_cast<uint32_t>((now_us - state_change_us_) / 1000ULL);

            switch (current_state_)
            {
            case MailboxState::EMPTY:
                // Detect new mail arriving
                if (!in_refractory && filtered_cm_ < trigger_thresh_cm_)
                {
                    if (!occluding_)
                    {
                        occlusion_start_us_ = now_us;
                        occluding_ = true;
                    }

                    const uint32_t held_ms = static_cast<uint32_t>((now_us - occlusion_start_us_) / 1000ULL);
                    if (held_ms >= Config::HOLD_MS)
                    {
                        // NEW MAIL DETECTED!
                        data.mail_detected = true;
                        data.delta_cm = baseline_cm_ - filtered_cm_;
                        data.duration_ms = held_ms;

                        current_state_ = MailboxState::HAS_MAIL;
                        state_change_us_ = now_us;
                        refractory_until_us_ = now_us + static_cast<uint64_t>(Config::REFRACTORY_MS) * 1000ULL;
                        occluding_ = false;

                        ESP_LOGI(LOG_TAG, "Mail detected! delta=%.2f cm, duration=%u ms, state: EMPTY->HAS_MAIL",
                                 data.delta_cm, data.duration_ms);
                    }
                }
                else if (occluding_)
                {
                    occluding_ = false;
                }
                break;

            case MailboxState::HAS_MAIL:
                // Check if mailbox is getting full
                if (filtered_cm_ < full_thresh_cm_)
                {
                    current_state_ = MailboxState::FULL;
                    state_change_us_ = now_us;
                    ESP_LOGI(LOG_TAG, "Mailbox full detected, state: HAS_MAIL->FULL");
                }
                // Check if mail was collected
                else if (filtered_cm_ > empty_thresh_cm_)
                {
                    if (!occluding_)
                    {
                        occlusion_start_us_ = now_us;
                        occluding_ = true;
                    }

                    const uint32_t held_ms = static_cast<uint32_t>((now_us - occlusion_start_us_) / 1000ULL);
                    if (held_ms >= Config::HOLD_MS)
                    {
                        // MAIL COLLECTED!
                        data.mail_collected = true;
                        data.delta_cm = filtered_cm_ - trigger_thresh_cm_;
                        data.duration_ms = held_ms;

                        current_state_ = MailboxState::EMPTIED;
                        state_change_us_ = now_us;
                        occluding_ = false;

                        ESP_LOGI(LOG_TAG, "Mail collected! delta=%.2f cm, duration=%u ms, state: HAS_MAIL->EMPTIED",
                                 data.delta_cm, data.duration_ms);
                    }
                }
                else if (occluding_)
                {
                    occluding_ = false;
                }
                break;

            case MailboxState::FULL:
                // Check if mail was collected
                if (filtered_cm_ > empty_thresh_cm_)
                {
                    if (!occluding_)
                    {
                        occlusion_start_us_ = now_us;
                        occluding_ = true;
                    }

                    const uint32_t held_ms = static_cast<uint32_t>((now_us - occlusion_start_us_) / 1000ULL);
                    if (held_ms >= Config::HOLD_MS)
                    {
                        // MAIL COLLECTED!
                        data.mail_collected = true;
                        data.delta_cm = filtered_cm_ - trigger_thresh_cm_;
                        data.duration_ms = held_ms;

                        current_state_ = MailboxState::EMPTIED;
                        state_change_us_ = now_us;
                        occluding_ = false;

                        ESP_LOGI(LOG_TAG, "Mail collected from full mailbox! delta=%.2f cm, duration=%u ms, state: FULL->EMPTIED",
                                 data.delta_cm, data.duration_ms);
                    }
                }
                else if (occluding_)
                {
                    occluding_ = false;
                }
                break;

            case MailboxState::EMPTIED:
                // Wait briefly in EMPTIED state, then transition to EMPTY
                if (time_in_state_ms >= Config::HOLD_MS)
                {
                    current_state_ = MailboxState::EMPTY;
                    state_change_us_ = now_us;
                    refractory_until_us_ = now_us + static_cast<uint64_t>(Config::REFRACTORY_MS) * 1000ULL;
                    ESP_LOGI(LOG_TAG, "Ready for new mail, state: EMPTIED->EMPTY");
                }
                break;
            }

            data.state = current_state_;
        }
    }
}