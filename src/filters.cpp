#include "ctm/filters.hpp"

#include <algorithm>
#include <stdexcept>

namespace ctm {

EmaFilter::EmaFilter(double smoothing_period) {
    if (smoothing_period < 1.0) {
        throw std::invalid_argument("EMA period must be >= 1");
    }
    alpha_ = 2.0 / (smoothing_period + 1.0);
}

double EmaFilter::process(double x) {
    if (!primed_) {
        ema_ = x;
        primed_ = true;
        return ema_;
    }
    ema_ = x * alpha_ + ema_ * (1.0 - alpha_);
    return ema_;
}

void EmaFilter::reset() {
    ema_ = 0.0;
    primed_ = false;
}

MovingAverage::MovingAverage(std::size_t window) : window_(window) {
    if (window < 1) {
        throw std::invalid_argument("moving average window must be >= 1");
    }
}

double MovingAverage::process(double x) {
    history_.push_back(x);
    sum_ += x;
    if (history_.size() > window_) {
        sum_ -= history_.front();
        history_.pop_front();
    }
    // На первых N отсчётах окно ещё не заполнено — нормируем на фактическую
    // длину, чтобы не получить заниженное значение при старте.
    return sum_ / static_cast<double>(history_.size());
}

void MovingAverage::reset() {
    history_.clear();
    sum_ = 0.0;
}

double FilterChain::process(double x) {
    double y = x;
    for (IFilter* stage : stages_) {
        y = stage->process(y);
    }
    return y;
}

void FilterChain::reset() {
    for (IFilter* stage : stages_) {
        stage->reset();
    }
}

} // namespace ctm
