#include "ctm/acquisition_pipeline.hpp"

#include <mutex>
#include <utility>

namespace ctm {

AcquisitionPipeline::AcquisitionPipeline(ISensorSource& source, PipelineConfig cfg)
    : source_(source),
      cfg_(cfg),
      sampler_(cfg.sample_rate_hz),
      ema_(cfg.ema_period),
      ma_(cfg.ma_window) {
    chain_.add(ma_);
    chain_.add(ema_);
}

AcquisitionPipeline::~AcquisitionPipeline() { stop(); }

void AcquisitionPipeline::start() {
    if (running_.exchange(true)) {
        return;
    }
    source_.start();
    sampler_.reset();
    source_thread_ = std::thread(&AcquisitionPipeline::source_loop, this);
}

void AcquisitionPipeline::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    if (source_thread_.joinable()) {
        source_thread_.join();
    }
    source_.stop();
}

void AcquisitionPipeline::source_loop() {
    while (running_.load(std::memory_order_relaxed)) {
        RawSample sample;
        if (!source_.read(sample)) {
            break;
        }
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            ++stats_.received;
        }
        if (buffer_.try_push(sample)) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            ++stats_.pushed;
        }
    }
}

void AcquisitionPipeline::run_for(std::chrono::milliseconds duration) {
    const auto deadline = Clock::now() + duration;
    while (Clock::now() < deadline) {
        TimePoint tick;
        if (!sampler_.wait_next(tick)) {
            continue;
        }

        RawSample latest;
        if (!buffer_.pop_latest(latest)) {
            continue; // источник ещё не успел выдать отсчёт
        }

        FilteredSample out;
        out.timestamp = tick;
        out.raw = latest.value;
        out.filtered = chain_.process(latest.value);
        out.seq = latest.seq;

        if (sink_) {
            sink_(out);
        }
        {
            std::lock_guard<std::mutex> lock(last_mutex_);
            last_ = out;
        }
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.processed;
    }

    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.overruns = buffer_.overruns();
    stats_.missed_deadlines = sampler_.missed();
}

PipelineStats AcquisitionPipeline::stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

FilteredSample AcquisitionPipeline::last() const {
    std::lock_guard<std::mutex> lock(last_mutex_);
    return last_;
}

} // namespace ctm
