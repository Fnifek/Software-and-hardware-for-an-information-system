#pragma once

#include "ctm/sensor_source.hpp"

#include <random>

namespace ctm {

// Параметры тепловой модели из лабораторной работы № 1:
// закон охлаждения Ньютона-Рихмана + аддитивный белый гауссовский шум.
struct ThermalModelConfig {
    double ambient_c{24.0};       // температура среды
    double tau_s{8.0};            // тепловая постоянная времени, с
    double load_period_s{45.0};   // период изменения нагрузки, с
    double idle_target_c{42.0};   // установившаяся температура при простое
    double load_target_c{78.0};   // установившаяся температура под нагрузкой
    double noise_sigma_c{0.35};   // СКО шума АЦП, °C
    double mains_amplitude_c{0.7};// амплитуда сетевой наводки, °C
    double mains_frequency_hz{49.5}; // частота наводки, Гц
    double adc_step_c{120.0 / 4095.0}; // шаг квантования 12-битного АЦП
    double source_rate_hz{200.0}; // частота опроса источника
    double jitter_sigma_s{0.0007};// СКО джиттера момента чтения, с
};

class ModelSensor final : public ISensorSource {
public:
    explicit ModelSensor(ThermalModelConfig cfg = {}, unsigned seed = 42);

    const SensorInfo& info() const override { return info_; }
    bool read(RawSample& out) override;
    void start() override;
    void stop() override;

    // Истинное (незашумлённое) значение — нужно только для верификации
    // качества фильтрации и в промышленном контуре не используется.
    double truth() const { return truth_; }

private:
    double target_temperature(double t) const;
    double quantize(double value) const;

    ThermalModelConfig cfg_;
    SensorInfo info_;
    std::mt19937 rng_;
    std::normal_distribution<double> noise_;
    std::normal_distribution<double> jitter_;
    double temperature_;
    double truth_;
    TimePoint t0_;
    TimePoint next_;
    std::uint32_t seq_{0};
    bool running_{false};
};

} // namespace ctm
