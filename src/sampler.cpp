#include "ctm/sampler.hpp"

#include <thread>

namespace ctm {

Sampler::Sampler(double fs_hz)
    : fs_hz_(fs_hz), period_(1.0 / fs_hz), next_(Clock::now()) {}

void Sampler::reset() {
    next_ = Clock::now();
    ticks_ = 0;
    missed_ = 0;
}

bool Sampler::wait_next(TimePoint& tick) {
    next_ += std::chrono::duration_cast<Clock::duration>(period_);
    std::this_thread::sleep_until(next_);

    const auto now = Clock::now();
    // Если опоздание превысило половину периода, сетка выборки уже
    // потеряна: фиксируем пропуск и привязываемся к текущему времени.
    if (now > next_ + std::chrono::duration_cast<Clock::duration>(period_ / 2)) {
        ++missed_;
        next_ = now;
        return false;
    }

    tick = next_;
    ++ticks_;
    return true;
}

void Sampler::wait_next_blocking() {
    next_ += std::chrono::duration_cast<Clock::duration>(period_);
    std::this_thread::sleep_until(next_);
    ++ticks_;
}

} // namespace ctm
