#include "ctm/filters.hpp"
#include "ctm/model_sensor.hpp"
#include "ctm/ring_buffer.hpp"
#include "ctm/sampler.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

int g_failures = 0;

void check(bool condition, const char* name) {
    if (condition) {
        std::cout << "[ OK ] " << name << '\n';
    } else {
        std::cout << "[FAIL] " << name << '\n';
        ++g_failures;
    }
}

bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) <= eps; }

void test_ring_buffer() {
    ctm::RingBuffer<int, 4> rb;
    check(rb.empty() && rb.size() == 0, "ring buffer starts empty");

    check(rb.try_push(1) && rb.try_push(2) && rb.try_push(3), "ring buffer push");
    check(rb.size() == 3, "ring buffer size after three pushes");

    check(!rb.try_push(4), "ring buffer rejects overflow");
    check(rb.overruns() == 1, "ring buffer counts overruns");

    int v = 0;
    check(rb.try_pop(v) && v == 1, "ring buffer FIFO order");
    check(rb.try_pop(v) && v == 2, "ring buffer FIFO order (2)");

    check(rb.try_push(5) && rb.try_push(6), "ring buffer wraps around");
    int latest = 0;
    check(rb.pop_latest(latest) && latest == 6, "pop_latest returns newest sample");
    check(rb.empty(), "pop_latest drains buffer");
}

void test_filters() {
    ctm::EmaFilter ema(10.0);
    check(near(ema.alpha(), 2.0 / 11.0), "EMA alpha = 2/(N+1)");
    double y = 0.0;
    for (int i = 0; i < 200; ++i) {
        y = ema.process(50.0);
    }
    check(near(y, 50.0, 1e-6), "EMA converges to constant input");

    ctm::MovingAverage ma(5);
    for (int i = 1; i <= 5; ++i) {
        y = ma.process(static_cast<double>(i));
    }
    check(near(y, 3.0), "moving average of 1..5 equals 3");

    ctm::EmaFilter ema2(10.0);
    ctm::MovingAverage ma2(5);
    ctm::FilterChain chain;
    chain.add(ma2);
    chain.add(ema2);
    check(near(chain.process(10.0), 10.0), "filter chain passes first sample");
    chain.reset();
    check(near(chain.process(1.0), 1.0), "filter chain reset");
}

void test_sampler() {
    ctm::Sampler sampler(100.0);
    ctm::TimePoint tick;
    int ticks = 0;
    const auto start = ctm::Clock::now();
    while (ctm::Clock::now() - start < std::chrono::milliseconds(300)) {
        if (sampler.wait_next(tick)) {
            ++ticks;
        }
    }
    check(ticks >= 25 && ticks <= 32, "sampler produces about 100 Hz");
}

void test_model() {
    ctm::ThermalModelConfig cfg;
    cfg.source_rate_hz = 1000.0;
    ctm::ModelSensor sensor(cfg);
    ctm::RawSample sample;
    bool in_range = true;
    for (int i = 0; i < 500; ++i) {
        sensor.read(sample);
        if (sample.value < 0.0 || sample.value > 120.0) {
            in_range = false;
        }
    }
    check(in_range, "model output stays inside ADC range");
    check(sample.seq == 499, "model sequence counter increments");
}

} // namespace

int main() {
    test_ring_buffer();
    test_filters();
    test_sampler();
    test_model();

    if (g_failures == 0) {
        std::cout << "All tests passed\n";
        return 0;
    }
    std::cout << g_failures << " test(s) failed\n";
    return 1;
}
