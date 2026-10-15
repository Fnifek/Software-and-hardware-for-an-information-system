#include "ctm/model_sensor.hpp"

#include <algorithm>
#include <cmath>
#include <thread>

namespace ctm {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

ModelSensor::ModelSensor(ThermalModelConfig cfg, unsigned seed)
    : cfg_(cfg),
      noise_(0.0, cfg.noise_sigma_c),
      jitter_(0.0, cfg.jitter_sigma_s),
      temperature_(cfg.idle_target_c),
      truth_(cfg.idle_target_c),
      t0_(Clock::now()),
      next_(t0_) {
    info_.id = "cpu_core_temp";
    info_.units = "degC";
    info_.kind = SensorKind::Model;
    info_.range_min = 0.0;
    info_.range_max = 120.0;
    info_.adc_bits = 12;
    rng_.seed(seed);
}

double ModelSensor::target_temperature(double t) const {
    // Прямоугольная нагрузка: половина периода — простой, половина — нагрузка.
    const double phase = std::fmod(t, cfg_.load_period_s);
    const bool loaded = phase >= cfg_.load_period_s * 0.5;
    if (loaded) {
        return cfg_.load_target_c;
    }
    return cfg_.idle_target_c;
}

double ModelSensor::quantize(double value) const {
    const double clipped = std::clamp(value, info_.range_min, info_.range_max);
    return std::round((clipped - info_.range_min) / cfg_.adc_step_c) * cfg_.adc_step_c +
           info_.range_min;
}

bool ModelSensor::read(RawSample& out) {
    if (!running_) {
        t0_ = Clock::now();
        next_ = t0_;
        running_ = true;
    }

    // Джиттер момента опроса имитирует неравномерность реального драйвера.
    const double jitter = std::clamp(jitter_(rng_), -0.003, 0.003);
    next_ += std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(1.0 / cfg_.source_rate_hz + jitter));
    std::this_thread::sleep_until(next_);

    const double t = std::chrono::duration<double>(next_ - t0_).count();
    const double target = target_temperature(t);
    // Точное решение уравнения теплового баланса на шаге dt:
    // T += (T_target - T) * (1 - exp(-dt / tau)).
    const double dt = 1.0 / cfg_.source_rate_hz;
    temperature_ += (target - temperature_) * (1.0 - std::exp(-dt / cfg_.tau_s));
    // Небольшая низкочастотная модуляция, имитирующая колебания оборотов кулера.
    const double ripple = 0.35 * std::sin(2.0 * kPi * 0.12 * t);
    // Сетевая наводка 49,5 Гц: при частоте выборки 100 Гц она попадает
    // почти на границу Найквиста и не подавляется простым усреднением.
    const double mains = cfg_.mains_amplitude_c *
                         std::sin(2.0 * kPi * cfg_.mains_frequency_hz * t);

    truth_ = temperature_ + ripple;
    out.timestamp = next_;
    out.value = quantize(truth_ + noise_(rng_) + mains);
    out.seq = seq_++;
    return true;
}

void ModelSensor::start() {
    running_ = false;
    seq_ = 0;
    temperature_ = cfg_.idle_target_c;
    truth_ = cfg_.idle_target_c;
}

void ModelSensor::stop() { running_ = false; }

} // namespace ctm
