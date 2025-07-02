#include "indicators/ParabolicSAR.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace backtrader {

ParabolicSAR::ParabolicSAR(std::shared_ptr<LineRoot> high_input,
                           std::shared_ptr<LineRoot> low_input,
                           double af_start, double af_increment, double af_maximum)
    : IndicatorBase(high_input, "ParabolicSAR"),
      af_start_(af_start),
      af_increment_(af_increment),
      af_maximum_(af_maximum),
      current_sar_(NaN),
      current_af_(af_start),
      extreme_point_(NaN),
      is_long_(true),
      initialized_(false) {
    
    if (af_start <= 0 || af_increment <= 0 || af_maximum <= af_start) {
        throw std::invalid_argument("Invalid Parabolic SAR parameters");
    }
    
    setInputs(high_input, low_input);
    setParam("af_start", af_start);
    setParam("af_increment", af_increment);
    setParam("af_maximum", af_maximum);
    setMinPeriod(2);  // Need at least 2 periods to establish trend
}

void ParabolicSAR::setInputs(std::shared_ptr<LineRoot> high_input, std::shared_ptr<LineRoot> low_input) {
    if (!high_input || !low_input) {
        throw std::invalid_argument("ParabolicSAR requires valid high and low inputs");
    }
    
    // Store additional input (base class stores first input)
    addInput(low_input);
}

void ParabolicSAR::reset() {
    IndicatorBase::reset();
    current_sar_ = NaN;
    current_af_ = af_start_;
    extreme_point_ = NaN;
    is_long_ = true;
    initialized_ = false;
    high_history_.clear();
    low_history_.clear();
}

void ParabolicSAR::calculate() {
    if (inputs_.size() < 2) {
        setOutput(0, NaN);
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    
    if (!high_input || !low_input) {
        setOutput(0, NaN);
        return;
    }
    
    double current_high = high_input->get(0);
    double current_low = low_input->get(0);
    
    if (isNaN(current_high) || isNaN(current_low)) {
        setOutput(0, NaN);
        return;
    }
    
    // Store initial values for initialization
    high_history_.push_back(current_high);
    low_history_.push_back(current_low);
    
    if (!initialized_) {
        if (high_history_.size() >= 2) {
            initialize(current_high, current_low);
        } else {
            setOutput(0, NaN);
            return;
        }
    } else {
        updateSAR(current_high, current_low);
    }
    
    setOutput(0, current_sar_);
}

void ParabolicSAR::calculateBatch(size_t start, size_t end) {
    if (inputs_.size() < 2) {
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    
    if (!high_input || !low_input) {
        return;
    }
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            high_input->forward();
            low_input->forward();
        }
    }
}

void ParabolicSAR::setAFStart(double af_start) {
    if (af_start <= 0) {
        throw std::invalid_argument("AF start must be positive");
    }
    af_start_ = af_start;
    setParam("af_start", af_start);
}

void ParabolicSAR::setAFIncrement(double af_increment) {
    if (af_increment <= 0) {
        throw std::invalid_argument("AF increment must be positive");
    }
    af_increment_ = af_increment;
    setParam("af_increment", af_increment);
}

void ParabolicSAR::setAFMaximum(double af_maximum) {
    if (af_maximum <= af_start_) {
        throw std::invalid_argument("AF maximum must be greater than AF start");
    }
    af_maximum_ = af_maximum;
    setParam("af_maximum", af_maximum);
}

void ParabolicSAR::setAFParameters(double af_start, double af_increment, double af_maximum) {
    if (af_start <= 0 || af_increment <= 0 || af_maximum <= af_start) {
        throw std::invalid_argument("Invalid Parabolic SAR parameters");
    }
    
    af_start_ = af_start;
    af_increment_ = af_increment;
    af_maximum_ = af_maximum;
    
    setParam("af_start", af_start);
    setParam("af_increment", af_increment);
    setParam("af_maximum", af_maximum);
}

double ParabolicSAR::getReversalSignal(std::shared_ptr<LineRoot> close_input) const {
    if (!close_input || isNaN(current_sar_)) {
        return 0.0;
    }
    
    try {
        double current_close = close_input->get(0);
        double prev_close = close_input->get(-1);
        double prev_sar = get(-1);
        
        if (isNaN(current_close) || isNaN(prev_close) || isNaN(prev_sar)) {
            return 0.0;
        }
        
        // Bullish reversal: price crosses above SAR
        if (prev_close <= prev_sar && current_close > current_sar_) {
            return 1.0;
        }
        
        // Bearish reversal: price crosses below SAR
        if (prev_close >= prev_sar && current_close < current_sar_) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double ParabolicSAR::getTrendDirection() const {
    if (!initialized_) {
        return 0.0;
    }
    
    return is_long_ ? 1.0 : -1.0;
}

double ParabolicSAR::getStopLoss() const {
    return current_sar_;
}

double ParabolicSAR::getDistanceToSAR(double current_price) const {
    if (isNaN(current_sar_) || current_price == 0.0) {
        return 0.0;
    }
    
    return std::abs(current_price - current_sar_) / current_price * 100.0;
}

bool ParabolicSAR::isAccelerating() const {
    return current_af_ > af_start_;
}

double ParabolicSAR::getSARMomentum(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 3;
        
        double sar_current = get(0);
        double sar_past = get(-static_cast<int>(lookback));
        
        if (isNaN(sar_current) || isNaN(sar_past)) {
            return 0.0;
        }
        
        return (sar_current - sar_past) / lookback;
        
    } catch (...) {
        return 0.0;
    }
}

void ParabolicSAR::initialize(double high, double low) {
    // Determine initial direction based on first two periods
    if (high_history_.size() >= 2 && low_history_.size() >= 2) {
        double prev_high = high_history_[high_history_.size() - 2];
        double prev_low = low_history_[low_history_.size() - 2];
        
        // Start with uptrend if current high > previous high
        is_long_ = (high > prev_high);
        
        if (is_long_) {
            current_sar_ = std::min(low, prev_low);
            extreme_point_ = std::max(high, prev_high);
        } else {
            current_sar_ = std::max(high, prev_high);
            extreme_point_ = std::min(low, prev_low);
        }
        
        current_af_ = af_start_;
        initialized_ = true;
    }
}

void ParabolicSAR::updateSAR(double high, double low) {
    // Check for reversal first
    if (checkReversal(high, low)) {
        reversePosition(high, low);
        return;
    }
    
    // Update extreme point and acceleration factor
    updateExtremePoint(high, low);
    
    // Calculate new SAR
    if (is_long_) {
        current_sar_ = current_sar_ + current_af_ * (extreme_point_ - current_sar_);
        
        // SAR cannot be above current or previous low
        current_sar_ = std::min(current_sar_, low);
        if (high_history_.size() >= 2) {
            double prev_low = low_history_[low_history_.size() - 2];
            current_sar_ = std::min(current_sar_, prev_low);
        }
    } else {
        current_sar_ = current_sar_ - current_af_ * (current_sar_ - extreme_point_);
        
        // SAR cannot be below current or previous high
        current_sar_ = std::max(current_sar_, high);
        if (high_history_.size() >= 2) {
            double prev_high = high_history_[high_history_.size() - 2];
            current_sar_ = std::max(current_sar_, prev_high);
        }
    }
}

bool ParabolicSAR::checkReversal(double high, double low) const {
    if (is_long_) {
        return low <= current_sar_;  // Bearish reversal
    } else {
        return high >= current_sar_; // Bullish reversal
    }
}

void ParabolicSAR::reversePosition(double high, double low) {
    is_long_ = !is_long_;
    current_af_ = af_start_;
    
    if (is_long_) {
        current_sar_ = extreme_point_;  // Previous extreme becomes new SAR
        extreme_point_ = high;          // Current high becomes new extreme
    } else {
        current_sar_ = extreme_point_;  // Previous extreme becomes new SAR
        extreme_point_ = low;           // Current low becomes new extreme
    }
}

void ParabolicSAR::updateExtremePoint(double high, double low) {
    bool updated_extreme = false;
    
    if (is_long_) {
        if (high > extreme_point_) {
            extreme_point_ = high;
            updated_extreme = true;
        }
    } else {
        if (low < extreme_point_) {
            extreme_point_ = low;
            updated_extreme = true;
        }
    }
    
    // Increase acceleration factor if extreme point was updated
    if (updated_extreme && current_af_ < af_maximum_) {
        current_af_ = std::min(current_af_ + af_increment_, af_maximum_);
    }
}

} // namespace backtrader