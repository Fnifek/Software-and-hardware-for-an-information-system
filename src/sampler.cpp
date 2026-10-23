#include "ctm/sampler.hpp"

#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace ctm {

Sampler::Sampler(double fs_hz)
    : fs_hz_(fs_hz), period_(1.0 / fs_hz), next_(Clock::now()) {
#ifdef _WIN32
    // Без повышения разрешения таймера Sleep округляет 10 мс до ~15,6 мс,
    // и блок дискретизации теряет каждый второй такт.
    timeBeginPeriod(1);
#endif
}

Sampler::~Sampler() {
#ifdef _WIN32
    timeEndPeriod(1);
#endif
}

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
