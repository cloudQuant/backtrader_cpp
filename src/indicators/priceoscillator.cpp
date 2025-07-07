#include "indicators/priceoscillator.h"
#include <limits>
#include <cmath>

namespace backtrader {

// PriceOscBase implementation
PriceOscBase::PriceOscBase() : Indicator() {
    _minperiod(std::max(params.period1, params.period2));
}

void PriceOscBase::prenext() {
    Indicator::prenext();
}

void PriceOscBase::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Calculate oscillator
    calculate_oscillator();
}

void PriceOscBase::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Calculate oscillator for all values
    for (int i = start; i < end; ++i) {
        calculate_oscillator();
    }
}

// PriceOscillator implementation
PriceOscillator::PriceOscillator() : PriceOscBase() {
    setup_lines();
}

PriceOscillator::PriceOscillator(std::shared_ptr<LineRoot> data_source, int period1, int period2) : PriceOscBase() {
    params.period1 = period1;
    params.period2 = period2;
    
    setup_lines();
    
    _minperiod(std::max(period1, period2));
    
    // Add data source to datas for traditional indicator interface
    if (data_source) {
        auto data_series = std::dynamic_pointer_cast<LineSeries>(data_source);
        if (data_series) {
            datas.push_back(data_series);
        }
    }
}

void PriceOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void PriceOscillator::calculate_oscillator() {
    // Simple implementation - difference between short and long EMAs
    // This is a placeholder - actual EMA calculation would be more complex
    auto po_line = lines->getline(po);
    if (po_line && datas.size() > 0 && datas[0]->lines) {
        auto data_line = datas[0]->lines->getline(0);
        if (data_line) {
            double current_price = (*data_line)[0];
            po_line->set(0, current_price);  // Placeholder
        }
    }
}

double PriceOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto line = lines->getline(po);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*line)[ago];
}

int PriceOscillator::getMinPeriod() const {
    return std::max(params.period1, params.period2);
}

void PriceOscillator::calculate() {
    next();
}

// PercentagePriceOscillator implementation
PercentagePriceOscillator::PercentagePriceOscillator(bool use_long_denominator) 
    : PriceOscBase(), use_long_denominator_(use_long_denominator) {
    setup_lines();
    
    _minperiod(std::max(params.period1, params.period2) + params.period_signal - 1);
}

PercentagePriceOscillator::PercentagePriceOscillator(std::shared_ptr<LineRoot> data_source, int period1, int period2, int period_signal)
    : PriceOscBase(), use_long_denominator_(true) {
    params.period1 = period1;
    params.period2 = period2;
    params.period_signal = period_signal;
    
    setup_lines();
    
    _minperiod(std::max(period1, period2) + period_signal - 1);
    
    // Add data source to datas for traditional indicator interface
    if (data_source) {
        auto data_series = std::dynamic_pointer_cast<LineSeries>(data_source);
        if (data_series) {
            datas.push_back(data_series);
        }
    }
    
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    signal_ema_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (period1 + 1.0);
    alpha2_ = 2.0 / (period2 + 1.0);
    alpha_signal_ = 2.0 / (period_signal + 1.0);
}

double PercentagePriceOscillator::get(int ago) const {
    return getPPOLine(ago);
}

int PercentagePriceOscillator::getMinPeriod() const {
    return std::max(params.period1, params.period2) + params.period_signal - 1;
}

void PercentagePriceOscillator::calculate() {
    next();
}

double PercentagePriceOscillator::getPPOLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto line = lines->getline(ppo);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*line)[ago];
}

double PercentagePriceOscillator::getSignalLine(int ago) const {
    if (!lines || lines->size() <= 1) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto line = lines->getline(signal);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*line)[ago];
}

double PercentagePriceOscillator::getHistogramLine(int ago) const {
    if (!lines || lines->size() <= 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto line = lines->getline(histo);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*line)[ago];
}

double PercentagePriceOscillator::getHistogram(int ago) const {
    return getHistogramLine(ago);
}

void PercentagePriceOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // PPO
        lines->add_line(std::make_shared<LineBuffer>());  // Signal
        lines->add_line(std::make_shared<LineBuffer>());  // Histogram
    }
}

void PercentagePriceOscillator::prenext() {
    PriceOscBase::prenext();
}

void PercentagePriceOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line) return;
    
    double current_price = (*data_line)[0];
    
    // Calculate EMAs
    if (first_calculation_) {
        ema1_value_ = current_price;
        ema2_value_ = current_price;
        first_calculation_ = false;
    } else {
        ema1_value_ = alpha1_ * current_price + (1.0 - alpha1_) * ema1_value_;
        ema2_value_ = alpha2_ * current_price + (1.0 - alpha2_) * ema2_value_;
    }
    
    // Calculate PPO
    double denominator = use_long_denominator_ ? ema2_value_ : ema1_value_;
    double ppo_value = 0.0;
    if (denominator != 0.0) {
        ppo_value = 100.0 * (ema1_value_ - ema2_value_) / denominator;
    }
    
    // Update PPO line
    auto ppo_line = lines->getline(ppo);
    if (ppo_line) {
        ppo_line->set(0, ppo_value);
    }
    
    // Calculate signal EMA
    if (data_line->size() >= static_cast<size_t>(std::max(params.period1, params.period2))) {
        if (data_line->size() == static_cast<size_t>(std::max(params.period1, params.period2))) {
            signal_ema_value_ = ppo_value;
        } else {
            signal_ema_value_ = alpha_signal_ * ppo_value + (1.0 - alpha_signal_) * signal_ema_value_;
        }
        
        // Update signal line
        auto signal_line = lines->getline(signal);
        if (signal_line) {
            signal_line->set(0, signal_ema_value_);
        }
        
        // Calculate histogram
        auto histo_line = lines->getline(histo);
        if (histo_line) {
            histo_line->set(0, ppo_value - signal_ema_value_);
        }
    }
}

void PercentagePriceOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Calculate PPO values for all positions
    for (int i = start; i < end; ++i) {
        next();
    }
}

void PercentagePriceOscillator::calculate_oscillator() {
    // This is called by the base class next(), but we override next() 
    // so this method is not needed for PPO
}

// PercentagePriceOscillatorShort implementation
PercentagePriceOscillatorShort::PercentagePriceOscillatorShort() 
    : PercentagePriceOscillator(false) {  // Use short MA as denominator
}

PercentagePriceOscillatorShort::PercentagePriceOscillatorShort(std::shared_ptr<LineRoot> data_source, int period1, int period2, int period_signal)
    : PercentagePriceOscillator(data_source, period1, period2, period_signal) {
    // Change to use short MA as denominator
    use_long_denominator_ = false;
}

} // namespace backtrader