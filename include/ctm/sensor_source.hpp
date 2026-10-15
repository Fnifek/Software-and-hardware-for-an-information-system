#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace ctm {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

// Тип источника данных. Позволяет подменять аппаратный датчик моделью
// без изменения остальных блоков конвейера (принцип инверсии зависимостей).
enum class SensorKind {
    HardwareAnalog,
    HardwareDigital,
    Model,
    RemoteApi
};

struct SensorInfo {
    std::string id;         // "cpu_core_temp"
    std::string units;      // "degC"
    SensorKind kind{SensorKind::Model};
    double range_min{0.0};  // нижняя граница шкалы АЦП
    double range_max{120.0};// верхняя граница шкалы АЦП
    unsigned adc_bits{12};  // разрядность АЦП
};

// Один отсчёт, уже приведённый к физическим единицам.
struct RawSample {
    TimePoint timestamp{};
    double value{0.0};
    std::uint32_t seq{0};
};

// Единый интерфейс любого источника сигнала: аналогового датчика,
// цифрового датчика, математической модели или внешнего API.
class ISensorSource {
public:
    virtual ~ISensorSource() = default;

    virtual const SensorInfo& info() const = 0;

    // Блокирующее чтение одного отсчёта. Возвращает false при отказе датчика.
    virtual bool read(RawSample& out) = 0;

    virtual void start() {}
    virtual void stop() {}
};

} // namespace ctm
