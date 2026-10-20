#pragma once

#include "ctm/filters.hpp"
#include "ctm/ring_buffer.hpp"
#include "ctm/sampler.hpp"
#include "ctm/sensor_source.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

namespace ctm {

struct PipelineConfig {
    double sample_rate_hz{100.0};   // частота выборки блока дискретизации
    double ema_period{7.0};         // N для EMA
    std::size_t ma_window{4};       // окно КИХ-фильтра
};

struct FilteredSample {
    TimePoint timestamp{};
    double raw{0.0};
    double filtered{0.0};
    std::uint32_t seq{0};
};

struct PipelineStats {
    std::uint64_t received{0};        // прочитано из источника
    std::uint64_t pushed{0};          // записано в буфер
    std::uint64_t overruns{0};        // переполнения буфера
    std::uint64_t processed{0};       // передано потребителю
    std::uint64_t missed_deadlines{0};// пропущенные такты дискретизации
};

// Конвейер приёма: источник -> кольцевой буфер -> дискретизация ->
// последовательность цифровых фильтров -> потребитель (БД, GUI, тесты).
class AcquisitionPipeline {
public:
    using Sink = std::function<void(const FilteredSample&)>;

    explicit AcquisitionPipeline(ISensorSource& source, PipelineConfig cfg = {});
    ~AcquisitionPipeline();

    AcquisitionPipeline(const AcquisitionPipeline&) = delete;
    AcquisitionPipeline& operator=(const AcquisitionPipeline&) = delete;

    void start();
    void stop();

    void set_sink(Sink sink) { sink_ = std::move(sink); }

    // Основной цикл дискретизации; блокирует поток на duration.
    void run_for(std::chrono::milliseconds duration);

    PipelineStats stats() const;
    FilteredSample last() const;

private:
    void source_loop();

    ISensorSource& source_;
    PipelineConfig cfg_;
    RingBuffer<RawSample, 4096> buffer_;
    Sampler sampler_;
    EmaFilter ema_;
    MovingAverage ma_;
    FilterChain chain_;
    Sink sink_;

    std::atomic<bool> running_{false};
    std::thread source_thread_;
    mutable std::mutex stats_mutex_;
    PipelineStats stats_;
    mutable std::mutex last_mutex_;
    FilteredSample last_;
};

} // namespace ctm
