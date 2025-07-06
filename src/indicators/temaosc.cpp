#include "indicators/temaosc.h"
#include "linebuffer.h"
#include <cmath>

namespace backtrader {
namespace indicators {

TripleExponentialMovingAverageOscillator::TripleExponentialMovingAverageOscillator() 
    : Indicator(), data_source_(nullptr), current_index_(0),
      fast_ema1_(0.0), fast_ema2_(0.0), fast_ema3_(0.0),
      slow_ema1_(0.0), slow_ema2_(0.0), slow_ema3_(0.0),
      first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    fast_alpha_ = 2.0 / (params.period1 + 1.0);
    fast_alpha1_ = 1.0 - fast_alpha_;
    slow_alpha_ = 2.0 / (params.period2 + 1.0);
    slow_alpha1_ = 1.0 - slow_alpha_;
}

TripleExponentialMovingAverageOscillator::TripleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0),
      fast_ema1_(0.0), fast_ema2_(0.0), fast_ema3_(0.0),
      slow_ema1_(0.0), slow_ema2_(0.0), slow_ema3_(0.0),
      first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    fast_alpha_ = 2.0 / (params.period1 + 1.0);
    fast_alpha1_ = 1.0 - fast_alpha_;
    slow_alpha_ = 2.0 / (params.period2 + 1.0);
    slow_alpha1_ = 1.0 - slow_alpha_;
}

TripleExponentialMovingAverageOscillator::TripleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period1, int period2)
    : Indicator(), data_source_(data_source), current_index_(0),
      fast_ema1_(0.0), fast_ema2_(0.0), fast_ema3_(0.0),
      slow_ema1_(0.0), slow_ema2_(0.0), slow_ema3_(0.0),
      first_run_(true) {
    params.period1 = period1;
    params.period2 = period2;
    setup_lines();
    
    // Calculate smoothing factors
    fast_alpha_ = 2.0 / (params.period1 + 1.0);
    fast_alpha1_ = 1.0 - fast_alpha_;
    slow_alpha_ = 2.0 / (params.period2 + 1.0);
    slow_alpha1_ = 1.0 - slow_alpha_;
}

TripleExponentialMovingAverageOscillator::TripleExponentialMovingAverageOscillator(std::shared_ptr<LineRoot> data)
    : Indicator(), data_source_(nullptr), current_index_(0),
      fast_ema1_(0.0), fast_ema2_(0.0), fast_ema3_(0.0),
      slow_ema1_(0.0), slow_ema2_(0.0), slow_ema3_(0.0),
      first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    fast_alpha_ = 2.0 / (params.period1 + 1.0);
    fast_alpha1_ = 1.0 - fast_alpha_;
    slow_alpha_ = 2.0 / (params.period2 + 1.0);
    slow_alpha1_ = 1.0 - slow_alpha_;
}

void TripleExponentialMovingAverageOscillator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double TripleExponentialMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return 0.0;
    }
    
    auto temaosc_line = lines->getline(temaosc);
    if (!temaosc_line) {
        return 0.0;
    }
    
    return (*temaosc_line)[ago];
}

int TripleExponentialMovingAverageOscillator::getMinPeriod() const {
    return std::max(params.period1, params.period2);
}

void TripleExponentialMovingAverageOscillator::calculate() {
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
        fast_ema1_ = fast_ema2_ = fast_ema3_ = price;
        slow_ema1_ = slow_ema2_ = slow_ema3_ = price;
        first_run_ = false;
    } else {
        // Calculate Fast TEMA (Triple EMA)
        fast_ema1_ = fast_alpha_ * price + fast_alpha1_ * fast_ema1_;
        fast_ema2_ = fast_alpha_ * fast_ema1_ + fast_alpha1_ * fast_ema2_;
        fast_ema3_ = fast_alpha_ * fast_ema2_ + fast_alpha1_ * fast_ema3_;
        
        // Calculate Slow TEMA (Triple EMA)
        slow_ema1_ = slow_alpha_ * price + slow_alpha1_ * slow_ema1_;
        slow_ema2_ = slow_alpha_ * slow_ema1_ + slow_alpha1_ * slow_ema2_;
        slow_ema3_ = slow_alpha_ * slow_ema2_ + slow_alpha1_ * slow_ema3_;
    }
    
    // Calculate TEMA values
    double fast_tema = 3.0 * fast_ema1_ - 3.0 * fast_ema2_ + fast_ema3_;
    double slow_tema = 3.0 * slow_ema1_ - 3.0 * slow_ema2_ + slow_ema3_;
    
    // Calculate oscillator (fast TEMA - slow TEMA)
    double oscillator = fast_tema - slow_tema;
    
    // Set the calculated value
    if (lines) {
        auto temaosc_line = lines->getline(temaosc);
        if (temaosc_line) {
            temaosc_line->set(0, oscillator);
        }
    }
    
    if (data_source_) {
        current_index_++;
    }
}

void TripleExponentialMovingAverageOscillator::next() {
    calculate();
}

void TripleExponentialMovingAverageOscillator::once(int start, int end) {
    // Reset state
    fast_ema1_ = fast_ema2_ = fast_ema3_ = 0.0;
    slow_ema1_ = slow_ema2_ = slow_ema3_ = 0.0;
    first_run_ = true;
    
    for (int i = start; i < end; ++i) {
        current_index_ = i;
        calculate();
    }
}

} // namespace indicators
} // namespace backtrader