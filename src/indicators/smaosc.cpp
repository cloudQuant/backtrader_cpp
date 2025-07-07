#include "indicators/smaosc.h"
#include "linebuffer.h"
#include <cmath>

namespace backtrader {
namespace indicators {

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator() 
    : Indicator(), data_source_(nullptr), current_index_(0),
      sum_(0.0), first_run_(true) {
    setup_lines();
}

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0),
      sum_(0.0), first_run_(true) {
    setup_lines();
}

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0),
      sum_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
}

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator(std::shared_ptr<LineRoot> data)
    : Indicator(), data_source_(nullptr), current_index_(0),
      sum_(0.0), first_run_(true) {
    setup_lines();
}

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator(std::shared_ptr<LineRoot> data, int period)
    : Indicator(), data_source_(nullptr), current_index_(0),
      sum_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
}

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator(std::shared_ptr<LineRoot> data, int fast, int slow)
    : Indicator(), data_source_(nullptr), current_index_(0),
      sum_(0.0), first_run_(true) {
    // Use the slower period as the oscillator period for compatibility
    params.period = slow;
    setup_lines();
}

void SimpleMovingAverageOscillator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double SimpleMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return 0.0;
    }
    
    auto smaosc_line = lines->getline(smaosc);
    if (!smaosc_line) {
        return 0.0;
    }
    
    return (*smaosc_line)[ago];
}

int SimpleMovingAverageOscillator::getMinPeriod() const {
    return params.period;
}

void SimpleMovingAverageOscillator::calculate() {
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
    
    // Add to price buffer
    price_buffer_.push_back(price);
    sum_ += price;
    if (price_buffer_.size() > static_cast<size_t>(params.period)) {
        sum_ -= price_buffer_.front();
        price_buffer_.pop_front();
    }
    
    // Calculate oscillator when we have enough data
    double oscillator = 0.0;
    if (price_buffer_.size() == static_cast<size_t>(params.period)) {
        double sma = sum_ / params.period;
        oscillator = price - sma;  // Price - SMA
    }
    
    // Set the calculated value
    if (lines) {
        auto smaosc_line = lines->getline(smaosc);
        if (smaosc_line) {
            smaosc_line->set(0, oscillator);
        }
    }
    
    if (data_source_) {
        current_index_++;
    }
}

void SimpleMovingAverageOscillator::next() {
    calculate();
}

void SimpleMovingAverageOscillator::once(int start, int end) {
    // Reset state
    price_buffer_.clear();
    sum_ = 0.0;
    first_run_ = true;
    
    for (int i = start; i < end; ++i) {
        current_index_ = i;
        calculate();
    }
}

} // namespace indicators
} // namespace backtrader