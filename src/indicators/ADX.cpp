#include "indicators/ADX.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace backtrader {

ADX::ADX(std::shared_ptr<LineRoot> high_input,
         std::shared_ptr<LineRoot> low_input,
         std::shared_ptr<LineRoot> close_input,
         size_t period)
    : IndicatorBase(high_input, "ADX"),
      period_(period),
      high_buffer_(period + 1),
      low_buffer_(period + 1),
      close_buffer_(period + 1),
      smoothed_tr_(0.0),
      smoothed_plus_dm_(0.0),
      smoothed_minus_dm_(0.0),
      smoothed_dx_(0.0),
      has_smoothed_values_(false),
      calculation_count_(0),
      dx_buffer_(period) {
    
    if (period == 0) {
        throw std::invalid_argument("ADX period must be greater than 0");
    }
    
    setInputs(high_input, low_input, close_input);
    setParam("period", static_cast<double>(period));
    setMinPeriod(period * 2 + 1);  // Need extra period for smoothing
}

void ADX::setInputs(std::shared_ptr<LineRoot> high_input,
                    std::shared_ptr<LineRoot> low_input,
                    std::shared_ptr<LineRoot> close_input) {
    
    if (!high_input || !low_input || !close_input) {
        throw std::invalid_argument("ADX requires valid high, low, and close inputs");
    }
    
    // Store additional inputs (base class stores first input)
    addInput(low_input);
    addInput(close_input);
}

void ADX::reset() {
    IndicatorBase::reset();
    high_buffer_.clear();
    low_buffer_.clear();
    close_buffer_.clear();
    dx_buffer_.clear();
    
    smoothed_tr_ = 0.0;
    smoothed_plus_dm_ = 0.0;
    smoothed_minus_dm_ = 0.0;
    smoothed_dx_ = 0.0;
    has_smoothed_values_ = false;
    calculation_count_ = 0;
}

void ADX::calculate() {
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
    
    if (high_buffer_.size() < 2) {
        setOutput(0, NaN);
        return;
    }
    
    // Get previous values
    double prev_high = high_buffer_[high_buffer_.size() - 2];
    double prev_low = low_buffer_[low_buffer_.size() - 2];
    double prev_close = close_buffer_[close_buffer_.size() - 2];
    
    // Calculate True Range
    double tr = calculateTrueRange(current_high, current_low, prev_close);
    
    // Calculate Directional Movement
    auto [plus_dm, minus_dm] = calculateDirectionalMovement(current_high, current_low, prev_high, prev_low);
    
    calculation_count_++;
    
    if (calculation_count_ == static_cast<int>(period_)) {
        // Initialize smoothed values with simple averages
        double sum_tr = 0.0, sum_plus_dm = 0.0, sum_minus_dm = 0.0;
        
        for (size_t i = 0; i < period_; ++i) {
            if (i + 1 < high_buffer_.size()) {
                double h = high_buffer_[i + 1];
                double l = low_buffer_[i + 1];
                double c_prev = close_buffer_[i];
                double h_prev = high_buffer_[i];
                double l_prev = low_buffer_[i];
                
                sum_tr += calculateTrueRange(h, l, c_prev);
                auto [pdm, mdm] = calculateDirectionalMovement(h, l, h_prev, l_prev);
                sum_plus_dm += pdm;
                sum_minus_dm += mdm;
            }
        }
        
        smoothed_tr_ = sum_tr;
        smoothed_plus_dm_ = sum_plus_dm;
        smoothed_minus_dm_ = sum_minus_dm;
        has_smoothed_values_ = true;
        
    } else if (has_smoothed_values_) {
        // Use Wilder's smoothing
        smoothed_tr_ = wilderSmoothing(tr, smoothed_tr_, period_);
        smoothed_plus_dm_ = wilderSmoothing(plus_dm, smoothed_plus_dm_, period_);
        smoothed_minus_dm_ = wilderSmoothing(minus_dm, smoothed_minus_dm_, period_);
    }
    
    if (!has_smoothed_values_ || smoothed_tr_ == 0.0) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate +DI and -DI
    double plus_di = (smoothed_plus_dm_ / smoothed_tr_) * 100.0;
    double minus_di = (smoothed_minus_dm_ / smoothed_tr_) * 100.0;
    
    // Calculate DX
    double di_sum = plus_di + minus_di;
    double dx = (di_sum != 0.0) ? (std::abs(plus_di - minus_di) / di_sum) * 100.0 : 0.0;
    
    dx_buffer_.push_back(dx);
    
    if (dx_buffer_.size() < period_) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate ADX as smoothed DX
    if (dx_buffer_.size() == period_) {
        // Initialize ADX with simple average of DX
        double sum_dx = 0.0;
        for (size_t i = 0; i < period_; ++i) {
            sum_dx += dx_buffer_[i];
        }
        smoothed_dx_ = sum_dx / period_;
    } else {
        // Use Wilder's smoothing for ADX
        smoothed_dx_ = wilderSmoothing(dx, smoothed_dx_, period_);
    }
    
    setOutput(0, smoothed_dx_);
}

void ADX::calculateBatch(size_t start, size_t end) {
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

void ADX::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("ADX period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period * 2 + 1);
    
    // Resize buffers and reset
    high_buffer_ = CircularBuffer<double>(period + 1);
    low_buffer_ = CircularBuffer<double>(period + 1);
    close_buffer_ = CircularBuffer<double>(period + 1);
    dx_buffer_ = CircularBuffer<double>(period);
    reset();
}

double ADX::getPlusDI() const {
    if (!has_smoothed_values_ || smoothed_tr_ == 0.0) {
        return NaN;
    }
    return (smoothed_plus_dm_ / smoothed_tr_) * 100.0;
}

double ADX::getMinusDI() const {
    if (!has_smoothed_values_ || smoothed_tr_ == 0.0) {
        return NaN;
    }
    return (smoothed_minus_dm_ / smoothed_tr_) * 100.0;
}

double ADX::getDX() const {
    if (dx_buffer_.empty()) {
        return NaN;
    }
    return dx_buffer_.back();
}

double ADX::getTrendStrength() const {
    double adx_value = get(0);
    if (isNaN(adx_value)) {
        return 0.0;
    }
    
    if (adx_value < 20) {
        return 0.25;  // Weak trend
    } else if (adx_value < 40) {
        return 0.5;   // Moderate trend
    } else if (adx_value < 60) {
        return 0.75;  // Strong trend
    } else {
        return 1.0;   // Very strong trend
    }
}

double ADX::getTrendDirection() const {
    double plus_di = getPlusDI();
    double minus_di = getMinusDI();
    
    if (isNaN(plus_di) || isNaN(minus_di)) {
        return 0.0;
    }
    
    if (plus_di > minus_di) {
        return 1.0;   // Uptrend
    } else if (minus_di > plus_di) {
        return -1.0;  // Downtrend
    } else {
        return 0.0;   // Neutral
    }
}

double ADX::getDICrossover() const {
    try {
        double plus_di_current = getPlusDI();
        double minus_di_current = getMinusDI();
        
        if (isNaN(plus_di_current) || isNaN(minus_di_current)) {
            return 0.0;
        }
        
        // Get previous DI values (would need to store history for this)
        // For now, return current direction
        if (plus_di_current > minus_di_current) {
            return 1.0;   // Bullish
        } else if (minus_di_current > plus_di_current) {
            return -1.0;  // Bearish
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

bool ADX::isTrending(double threshold) const {
    double adx_value = get(0);
    return !isNaN(adx_value) && adx_value > threshold;
}

bool ADX::isTrendStrengthening() const {
    try {
        double adx_current = get(0);
        double adx_prev = get(-1);
        
        if (isNaN(adx_current) || isNaN(adx_prev)) {
            return false;
        }
        
        return adx_current > adx_prev;
        
    } catch (...) {
        return false;
    }
}

double ADX::getADXSlope(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 3;
        
        double adx_current = get(0);
        double adx_past = get(-static_cast<int>(lookback));
        
        if (isNaN(adx_current) || isNaN(adx_past)) {
            return 0.0;
        }
        
        return (adx_current - adx_past) / lookback;
        
    } catch (...) {
        return 0.0;
    }
}

double ADX::getVolatility() const {
    if (!has_smoothed_values_) {
        return NaN;
    }
    return smoothed_tr_;
}

double ADX::calculateTrueRange(double high, double low, double prev_close) const {
    double tr1 = high - low;
    double tr2 = std::abs(high - prev_close);
    double tr3 = std::abs(low - prev_close);
    
    return std::max({tr1, tr2, tr3});
}

std::pair<double, double> ADX::calculateDirectionalMovement(double high, double low,
                                                           double prev_high, double prev_low) const {
    double up_move = high - prev_high;
    double down_move = prev_low - low;
    
    double plus_dm = 0.0;
    double minus_dm = 0.0;
    
    if (up_move > down_move && up_move > 0) {
        plus_dm = up_move;
    } else if (down_move > up_move && down_move > 0) {
        minus_dm = down_move;
    }
    
    return {plus_dm, minus_dm};
}

double ADX::wilderSmoothing(double current_value, double previous_smoothed, size_t period) const {
    return previous_smoothed + (current_value - previous_smoothed) / period;
}

} // namespace backtrader