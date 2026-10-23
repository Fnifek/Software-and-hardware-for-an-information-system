#pragma once

#include "ctm/sensor_source.hpp"

#include <chrono>
#include <cstdint>

namespace ctm {

// Блок дискретизации: формирует равномерную сетку моментов выборки
// с частотой fs_hz и компенсирует накопление ошибки часов.
// Опорное время — steady_clock, поэтому системное время на него не влияет.
class Sampler {
public:
    explicit Sampler(double fs_hz);
    ~Sampler();

    // Ожидает очередной такт. Возвращает true, если такт выдержан,
    // и false, если потребитель опоздал (такт пропущен, сетка восстановлена).
    bool wait_next(TimePoint& tick);

    // Блокирующее ожидание без пропуска тактов — используется в тестах.
    void wait_next_blocking();

    double fs() const { return fs_hz_; }
    double period_s() const { return 1.0 / fs_hz_; }
    std::uint64_t ticks() const { return ticks_; }
    std::uint64_t missed() const { return missed_; }
    void reset();

private:
    double fs_hz_;
    std::chrono::duration<double> period_;
    TimePoint next_;
    std::uint64_t ticks_{0};
    std::uint64_t missed_{0};
};

} // namespace ctm
