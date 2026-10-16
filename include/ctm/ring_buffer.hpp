#pragma once

#include "ctm/sensor_source.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ctm {

// Кольцевой буфер SPSC (single producer / single consumer).
// Писатель — поток приёма данных, читатель — поток дискретизации.
// Политика переполнения: новый отсчёт отбрасывается, счётчик переполнений
// увеличивается, что позволяет диагностировать недостаточную производительность
// потребителя. Все индексы обновляются с ослабленной/приобретающей семантикой,
// поэтому блокировки в горячем пути не используются.
template <typename T, std::size_t Capacity>
class RingBuffer {
    static_assert(Capacity >= 2, "capacity must be at least 2");
    static_assert((Capacity & (Capacity - 1)) == 0, "capacity must be a power of two");

public:
    RingBuffer() = default;
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    bool try_push(const T& item) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto next = (head + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire)) {
            overruns_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        slots_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    bool try_pop(T& out) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        out = slots_[tail];
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return true;
    }

    // Забирает самый свежий отсчёт, отбрасывая устаревшие. Именно так
    // работает блок дискретизации: потребителю важно текущее состояние,
    // а не вся история, накопленная за пропущенный такт.
    bool pop_latest(T& out) noexcept {
        bool got = false;
        T tmp{};
        while (try_pop(tmp)) {
            out = tmp;
            got = true;
        }
        return got;
    }

    std::size_t size() const noexcept {
        const auto head = head_.load(std::memory_order_acquire);
        const auto tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & mask_;
    }

    bool empty() const noexcept { return size() == 0; }
    std::uint64_t overruns() const noexcept { return overruns_.load(std::memory_order_relaxed); }

private:
    static constexpr std::size_t mask_ = Capacity - 1;
    std::vector<T> slots_{Capacity};
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
    std::atomic<std::uint64_t> overruns_{0};
};

} // namespace ctm
