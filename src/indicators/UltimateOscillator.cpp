#include "indicators/UltimateOscillator.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace backtrader {

UltimateOscillator::UltimateOscillator(std::shared_ptr<LineRoot> high_input,
                                       std::shared_ptr<LineRoot> low_input,
                                       std::shared_ptr<LineRoot> close_input,
                                       size_t period1, size_t period2, size_t period3)
    : IndicatorBase(high_input, "UltimateOscillator"),
      period1_(period1),
      period2_(period2),
      period3_(period3),
      high_buffer_(period3 + 1),
      low_buffer_(period3 + 1),
      close_buffer_(period3 + 1),
      bp_buffer_(period3),
      tr_buffer_(period3),
      bp_sum1_(0.0), bp_sum2_(0.0), bp_sum3_(0.0),
      tr_sum1_(0.0), tr_sum2_(0.0), tr_sum3_(0.0) {
    
    if (period1 == 0 || period2 == 0 || period3 == 0) {
        throw std::invalid_argument("Ultimate Oscillator periods must be greater than 0");
    }
    
    if (period1 >= period2 || period2 >= period3) {
        throw std::invalid_argument("Ultimate Oscillator periods must be in ascending order");
    }
    
    setInputs(high_input, low_input, close_input);
    setParam("period1", static_cast<double>(period1));
    setParam("period2", static_cast<double>(period2));
    setParam("period3", static_cast<double>(period3));
    setMinPeriod(period3 + 1);
}

void UltimateOscillator::setInputs(std::shared_ptr<LineRoot> high_input,
                                   std::shared_ptr<LineRoot> low_input,
                                   std::shared_ptr<LineRoot> close_input) {
    if (!high_input || !low_input || !close_input) {
        throw std::invalid_argument("Ultimate Oscillator requires valid high, low, and close inputs");
    }
    
    // Store additional inputs (base class stores first input)
    addInput(low_input);
    addInput(close_input);
}

void UltimateOscillator::reset() {
    IndicatorBase::reset();
    high_buffer_.clear();
    low_buffer_.clear();
    close_buffer_.clear();
    bp_buffer_.clear();
    tr_buffer_.clear();
    
    bp_sum1_ = bp_sum2_ = bp_sum3_ = 0.0;
    tr_sum1_ = tr_sum2_ = tr_sum3_ = 0.0;
}

void UltimateOscillator::calculate() {
    if (inputs_.size() < 3) {
        setOutput(0, NaN);
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    auto close_input = getInput(2);
    
    if (!high_input || !low_input || !close_input) {
        setOutput(0, NaN);
        return;
    }
    
    double current_high = high_input->get(0);
    double current_low = low_input->get(0);
    double current_close = close_input->get(0);
    
    if (isNaN(current_high) || isNaN(current_low) || isNaN(current_close)) {
        setOutput(0, NaN);
        return;
    }
    
    // Add current values to buffers
    high_buffer_.push_back(current_high);
    low_buffer_.push_back(current_low);
    close_buffer_.push_back(current_close);
    
    if (close_buffer_.size() < 2) {
        setOutput(0, NaN);
        return;
    }
    
    // Get previous close
    double prev_close = close_buffer_[close_buffer_.size() - 2];
    
    // Calculate Buying Pressure and True Range
    double bp = calculateBuyingPressure(current_close, current_low, prev_close);
    double tr = calculateTrueRange(current_high, current_low, prev_close);
    
    bp_buffer_.push_back(bp);
    tr_buffer_.push_back(tr);
    
    // Update sums
    updateSums(bp, tr);
    
    if (bp_buffer_.size() < period3_) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate averages for each period
    double avg1 = (tr_sum1_ != 0.0) ? bp_sum1_ / tr_sum1_ : 0.0;
    double avg2 = (tr_sum2_ != 0.0) ? bp_sum2_ / tr_sum2_ : 0.0;
    double avg3 = (tr_sum3_ != 0.0) ? bp_sum3_ / tr_sum3_ : 0.0;
    
    // Calculate Ultimate Oscillator
    double uo = 100.0 * (4.0 * avg1 + 2.0 * avg2 + avg3) / 7.0;
    
    setOutput(0, uo);
}

void UltimateOscillator::calculateBatch(size_t start, size_t end) {
    if (inputs_.size() < 3) {
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    auto close_input = getInput(2);
    
    if (!high_input || !low_input || !close_input) {
        return;
    }
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            high_input->forward();
            low_input->forward();
            close_input->forward();
        }
    }
}

void UltimateOscillator::setPeriods(size_t period1, size_t period2, size_t period3) {
    if (period1 == 0 || period2 == 0 || period3 == 0) {
        throw std::invalid_argument("Ultimate Oscillator periods must be greater than 0");
    }
    
    if (period1 >= period2 || period2 >= period3) {
        throw std::invalid_argument("Ultimate Oscillator periods must be in ascending order");
    }
    
    period1_ = period1;
    period2_ = period2;
    period3_ = period3;
    
    setParam("period1", static_cast<double>(period1));
    setParam("period2", static_cast<double>(period2));
    setParam("period3", static_cast<double>(period3));
    setMinPeriod(period3 + 1);
    
    // Resize buffers and reset
    high_buffer_ = CircularBuffer<double>(period3 + 1);
    low_buffer_ = CircularBuffer<double>(period3 + 1);
    close_buffer_ = CircularBuffer<double>(period3 + 1);
    bp_buffer_ = CircularBuffer<double>(period3);
    tr_buffer_ = CircularBuffer<double>(period3);
    reset();
}

double UltimateOscillator::getOverboughtOversold(double overbought_level, double oversold_level) const {
    double uo_value = get(0);
    if (isNaN(uo_value)) {
        return 0.0;
    }
    
    if (uo_value >= overbought_level) {
        return 1.0;   // Overbought
    } else if (uo_value <= oversold_level) {
        return -1.0;  // Oversold
    } else {
        return 0.0;   // Neutral
    }
}

double UltimateOscillator::getDivergenceSignal() const {
    try {
        double uo_current = get(0);
        double uo_prev5 = get(-5);
        double uo_prev10 = get(-10);
        
        if (isNaN(uo_current) || isNaN(uo_prev5) || isNaN(uo_prev10)) {
            return 0.0;
        }
        
        // Simple divergence detection based on trend changes
        bool uo_rising = uo_current > uo_prev5 && uo_prev5 > uo_prev10;
        bool uo_falling = uo_current < uo_prev5 && uo_prev5 < uo_prev10;
        
        if (uo_rising && uo_current < 50.0) {
            return 1.0;   // Bullish divergence from oversold area
        } else if (uo_falling && uo_current > 50.0) {
            return -1.0;  // Bearish divergence from overbought area
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double UltimateOscillator::getMomentum(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 3;
        
        double uo_current = get(0);
        double uo_past = get(-static_cast<int>(lookback));
        
        if (isNaN(uo_current) || isNaN(uo_past)) {
            return 0.0;
        }
        
        return uo_current - uo_past;
        
    } catch (...) {
        return 0.0;
    }
}

double UltimateOscillator::getBuyingPressure() const {
    if (bp_buffer_.empty()) {
        return NaN;
    }
    return bp_buffer_.back();
}

double UltimateOscillator::getTrueRange() const {
    if (tr_buffer_.empty()) {
        return NaN;
    }
    return tr_buffer_.back();
}

double UltimateOscillator::getTrendStrength() const {
    double uo_value = get(0);
    if (isNaN(uo_value)) {
        return 0.0;
    }
    
    // Distance from neutral (50) indicates trend strength
    double distance_from_neutral = std::abs(uo_value - 50.0);
    return (distance_from_neutral / 50.0) * 100.0;
}

double UltimateOscillator::calculateBuyingPressure(double close, double low, double prev_close) const {
    double true_low = std::min(low, prev_close);
    return close - true_low;
}

double UltimateOscillator::calculateTrueRange(double high, double low, double prev_close) const {
    double true_high = std::max(high, prev_close);
    double true_low = std::min(low, prev_close);
    return true_high - true_low;
}

void UltimateOscillator::updateSums(double bp, double tr) {
    // Update period 1 sums
    bp_sum1_ += bp;
    tr_sum1_ += tr;
    if (bp_buffer_.size() > period1_) {
        bp_sum1_ -= bp_buffer_[bp_buffer_.size() - period1_ - 1];
        tr_sum1_ -= tr_buffer_[tr_buffer_.size() - period1_ - 1];
    }
    
    // Update period 2 sums
    bp_sum2_ += bp;
    tr_sum2_ += tr;
    if (bp_buffer_.size() > period2_) {
        bp_sum2_ -= bp_buffer_[bp_buffer_.size() - period2_ - 1];
        tr_sum2_ -= tr_buffer_[tr_buffer_.size() - period2_ - 1];
    }
    
    // Update period 3 sums
    bp_sum3_ += bp;
    tr_sum3_ += tr;
    if (bp_buffer_.size() > period3_) {
        bp_sum3_ -= bp_buffer_[bp_buffer_.size() - period3_ - 1];
        tr_sum3_ -= tr_buffer_[tr_buffer_.size() - period3_ - 1];
    }
}

} // namespace backtrader