#include "indicators/emaosc.h"
#include "linebuffer.h"
#include <cmath>

namespace backtrader {
namespace indicators {

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator() 
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema_(0.0), first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0),
      ema_(0.0), first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0),
      ema_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator(std::shared_ptr<LineRoot> data)
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema_(0.0), first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator(std::shared_ptr<LineRoot> data, int period)
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator(std::shared_ptr<LineRoot> data, int fast, int slow)
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema_(0.0), first_run_(true) {
    // Use the slower period as the oscillator period for compatibility
    params.period = slow;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

void ExponentialMovingAverageOscillator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double ExponentialMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return 0.0;
    }
    
    auto emaosc_line = lines->getline(emaosc);
    if (!emaosc_line) {
        return 0.0;
    }
    
    return (*emaosc_line)[ago];
}

int ExponentialMovingAverageOscillator::getMinPeriod() const {
    return params.period;
}

void ExponentialMovingAverageOscillator::calculate() {
    double price;
    
    // Get price data
    if (data_source_) {
        if (current_index_ >= data_source_->size()) return;
        price = (*data_source_)[current_index_];
    } else if (!datas.empty() && datas[0]->lines) {
        auto data_line = datas[0]->lines->getline(0);
        if (!data_line) return;
        price = (*data_line)[0];
    } else {
        return;
    }
    
    if (first_run_) {
        // Initialize with the first price
        ema_ = price;
        first_run_ = false;
    } else {
        // Calculate EMA
        ema_ = alpha_ * price + alpha1_ * ema_;
    }
    
    // Calculate oscillator (price - EMA)
    double oscillator = price - ema_;
    
    // Set the calculated value
    if (lines) {
        auto emaosc_line = lines->getline(emaosc);
        if (emaosc_line) {
            emaosc_line->set(0, oscillator);
        }
    }
    
    if (data_source_) {
        current_index_++;
    }
}

void ExponentialMovingAverageOscillator::next() {
    calculate();
}

void ExponentialMovingAverageOscillator::once(int start, int end) {
    // Reset state
    ema_ = 0.0;
    first_run_ = true;
    
    for (int i = start; i < end; ++i) {
        current_index_ = i;
        calculate();
    }
}

} // namespace indicators
} // namespace backtrader