#include "processor.hpp"

namespace Processor
{
    Processor::Processor()
        : baseline_cm_(Config::BASELINE_CM),
          trigger_thresh_cm_(Config::BASELINE_CM - Config::TRIGGER_DELTA_CM),
          full_thresh_cm_(Config::BASELINE_CM - (2.0f * Config::TRIGGER_DELTA_CM)),
          empty_thresh_cm_(Config::BASELINE_CM - (Config::TRIGGER_DELTA_CM * 0.5f))
    {
        ctx_ = {};
        ctx_.current_state = MailboxState::EMPTY;
        ctx_.filtered_cm = -1.0f;

        ESP_LOGI(LOG_TAG, "Processor initialized. baseline=%.2f cm, trigger=%.2f cm, full=%.2f cm, empty=%.2f cm",
                 baseline_cm_, trigger_thresh_cm_, full_thresh_cm_, empty_thresh_cm_);
    }

    Processor::Processor(const StateContext &ctx)
        : ctx_(ctx),
          baseline_cm_(Config::BASELINE_CM),
          trigger_thresh_cm_(Config::BASELINE_CM - Config::TRIGGER_DELTA_CM),
          full_thresh_cm_(Config::BASELINE_CM - (2.0f * Config::TRIGGER_DELTA_CM)),
          empty_thresh_cm_(Config::BASELINE_CM - (Config::TRIGGER_DELTA_CM * 0.5f))
    {
        ESP_LOGI(LOG_TAG, "Processor initialized. baseline=%.2f cm, trigger=%.2f cm, full=%.2f cm, empty=%.2f cm",
                 baseline_cm_, trigger_thresh_cm_, full_thresh_cm_, empty_thresh_cm_);
    }

    DistanceData Processor::Process(const float raw_distance_cm, const uint64_t current_time_us)
    {
        DistanceData data = {};
        data.raw_cm = raw_distance_cm;
        data.mail_detected = false;
        data.mail_collected = false;
        data.state = ctx_.current_state;

        // Track success rate
        ctx_.total_count++;
        if (raw_distance_cm > 0)
            ctx_.ok_count++;

        // Filter the measurement
        addToFilter(raw_distance_cm);
        data.filtered_cm = ctx_.filtered_cm;

        // Update success rate periodically
        const uint32_t elapsed_ms = static_cast<uint32_t>((current_time_us - ctx_.last_update_us) / 1000ULL);
        if (elapsed_ms >= 1000) // Update every second
        {
            updateSuccessRate(elapsed_ms);
            ctx_.last_update_us = current_time_us;
        }
        data.success_rate = ctx_.success_rate;

        // Run state machine
        updateStateMachine(data, current_time_us);

        return data;
    }

    StateContext Processor::GetContext() const { return ctx_; }
    float Processor::GetBaseline() const { return baseline_cm_; }
    float Processor::GetThreshold() const { return trigger_thresh_cm_; }
    float Processor::GetFullThreshold() const { return full_thresh_cm_; }
    bool Processor::InRefractory(const uint64_t current_time_us) const { return current_time_us < ctx_.refractory_until_us; }
    MailboxState Processor::GetState() const { return ctx_.current_state; }

    void Processor::addToFilter(const float &distance_cm)
    {
        ctx_.window[ctx_.w_idx] = distance_cm;
        ctx_.w_idx = (ctx_.w_idx + 1) % Config::FILTER_WINDOW;
        if (ctx_.w_count < Config::FILTER_WINDOW)
            ctx_.w_count++;

        ctx_.filtered_cm = calculateMedian();
    }

    float Processor::calculateMedian() const
    {
        float tmp[Config::FILTER_WINDOW];
        size_t n = 0;
        for (size_t i = 0; i < ctx_.w_count; ++i)
        {
            const float v = ctx_.window[i];
            if (v > 0)
                tmp[n++] = v;
        }
        if (n == 0)
            return -1.0f;

        std::sort(tmp, tmp + n);
        return (n & 1) ? tmp[n / 2] : 0.5f * (tmp[n / 2 - 1] + tmp[n / 2]);
    }

    void Processor::updateSuccessRate(const uint32_t &elapsed_ms)
    {
        ctx_.success_rate = (ctx_.total_count > 0)
                                ? static_cast<float>(ctx_.ok_count) / static_cast<float>(ctx_.total_count)
                                : 0.0f;

        ctx_.ms_since_decay += elapsed_ms;
        if (ctx_.ms_since_decay >= 60000)
        {
            ctx_.ok_count = ctx_.ok_count / 2;
            ctx_.total_count = ctx_.total_count / 2;
            ctx_.ms_since_decay = 0;
        }
    }

    void Processor::updateStateMachine(DistanceData &data, const uint64_t now_us)
    {
        // Invalid reading - maintain current state
        if (ctx_.filtered_cm <= 0)
            return;

        const bool in_refractory = (now_us < ctx_.refractory_until_us);
        const uint32_t time_in_state_ms = static_cast<uint32_t>((now_us - ctx_.state_change_us) / 1000ULL);

        switch (ctx_.current_state)
        {
        case MailboxState::EMPTY:
            // Detect new mail arriving
            if (!in_refractory && ctx_.filtered_cm < trigger_thresh_cm_)
            {
                if (!ctx_.occluding)
                {
                    ctx_.occlusion_start_us = now_us;
                    ctx_.occluding = true;
                }

                const uint32_t held_ms = static_cast<uint32_t>((now_us - ctx_.occlusion_start_us) / 1000ULL);
                if (held_ms >= Config::HOLD_MS)
                {
                    // NEW MAIL DETECTED!
                    data.mail_detected = true;
                    data.delta_cm = baseline_cm_ - ctx_.filtered_cm;
                    data.duration_ms = held_ms;

                    ctx_.current_state = MailboxState::HAS_MAIL;
                    ctx_.state_change_us = now_us;
                    ctx_.refractory_until_us = now_us + static_cast<uint64_t>(Config::REFRACTORY_MS) * 1000ULL;
                    ctx_.occluding = false;

                    ESP_LOGI(LOG_TAG, "Mail detected! delta=%.2f cm, duration=%u ms, state: EMPTY->HAS_MAIL",
                             data.delta_cm, data.duration_ms);
                }
            }
            else if (ctx_.occluding)
            {
                ctx_.occluding = false;
            }
            break;

        case MailboxState::HAS_MAIL:
            // Check if mailbox is getting full
            if (ctx_.filtered_cm < full_thresh_cm_)
            {
                ctx_.current_state = MailboxState::FULL;
                ctx_.state_change_us = now_us;
                ESP_LOGI(LOG_TAG, "Mailbox full detected, state: HAS_MAIL->FULL");
            }
            // Check if mail was collected
            else if (ctx_.filtered_cm > empty_thresh_cm_)
            {
                if (!ctx_.occluding)
                {
                    ctx_.occlusion_start_us = now_us;
                    ctx_.occluding = true;
                }

                const uint32_t held_ms = static_cast<uint32_t>((now_us - ctx_.occlusion_start_us) / 1000ULL);
                if (held_ms >= Config::HOLD_MS)
                {
                    // MAIL COLLECTED!
                    data.mail_collected = true;
                    data.delta_cm = ctx_.filtered_cm - trigger_thresh_cm_;
                    data.duration_ms = held_ms;

                    ctx_.current_state = MailboxState::EMPTIED;
                    ctx_.state_change_us = now_us;
                    ctx_.occluding = false;

                    ESP_LOGI(LOG_TAG, "Mail collected! delta=%.2f cm, duration=%u ms, state: HAS_MAIL->EMPTIED",
                             data.delta_cm, data.duration_ms);
                }
            }
            else if (ctx_.occluding)
            {
                ctx_.occluding = false;
            }
            break;

        case MailboxState::FULL:
            // Check if mail was collected
            if (ctx_.filtered_cm > empty_thresh_cm_)
            {
                if (!ctx_.occluding)
                {
                    ctx_.occlusion_start_us = now_us;
                    ctx_.occluding = true;
                }

                const uint32_t held_ms = static_cast<uint32_t>((now_us - ctx_.occlusion_start_us) / 1000ULL);
                if (held_ms >= Config::HOLD_MS)
                {
                    // MAIL COLLECTED!
                    data.mail_collected = true;
                    data.delta_cm = ctx_.filtered_cm - trigger_thresh_cm_;
                    data.duration_ms = held_ms;

                    ctx_.current_state = MailboxState::EMPTIED;
                    ctx_.state_change_us = now_us;
                    ctx_.occluding = false;

                    ESP_LOGI(LOG_TAG, "Mail collected from full mailbox! delta=%.2f cm, duration=%u ms, state: FULL->EMPTIED",
                             data.delta_cm, data.duration_ms);
                }
            }
            else if (ctx_.occluding)
            {
                ctx_.occluding = false;
            }
            break;

        case MailboxState::EMPTIED:
            // Wait briefly in EMPTIED state, then transition to EMPTY
            if (time_in_state_ms >= Config::HOLD_MS)
            {
                ctx_.current_state = MailboxState::EMPTY;
                ctx_.state_change_us = now_us;
                ctx_.refractory_until_us = now_us + static_cast<uint64_t>(Config::REFRACTORY_MS) * 1000ULL;
                ESP_LOGI(LOG_TAG, "Ready for new mail, state: EMPTIED->EMPTY");
            }
            break;
        }

        data.state = ctx_.current_state;
    }
}