#pragma once

#include <cstddef>
#include <deque>
#include <vector>

namespace ctm {

// Интерфейс цифрового фильтра: один вход — один выход (SISO).
class IFilter {
public:
    virtual ~IFilter() = default;
    virtual double process(double x) = 0;
    virtual void reset() = 0;
};

// Экспоненциальное скользящее среднее:
//   EMA = P * alpha + EMA_prev * (1 - alpha),  alpha = 2 / (N + 1).
class EmaFilter final : public IFilter {
public:
    explicit EmaFilter(double smoothing_period);

    double process(double x) override;
    void reset() override;

    double alpha() const { return alpha_; }
    double value() const { return ema_; }

private:
    double alpha_;
    double ema_{0.0};
    bool primed_{false};
};

// Скользящее среднее по окну N (КИХ-фильтр с равномерными весами):
//   Y[n] = (1/N) * sum_{j=0..N-1} X[n-j].
class MovingAverage final : public IFilter {
public:
    explicit MovingAverage(std::size_t window);

    double process(double x) override;
    void reset() override;

    std::size_t window() const { return window_; }

private:
    std::size_t window_;
    std::deque<double> history_;
    double sum_{0.0};
};

// Последовательное соединение фильтров. Порядок важен: сначала КИХ
// подавляет высокочастотные помехи, затем EMA сглаживает остаток тренда.
class FilterChain {
public:
    void add(IFilter& filter) { stages_.push_back(&filter); }
    double process(double x);
    void reset();

private:
    std::vector<IFilter*> stages_;
};

} // namespace ctm
