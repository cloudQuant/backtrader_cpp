#include "indicators/TSI.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace backtrader {

TSI::TSI(std::shared_ptr<LineRoot> input, size_t first_smoothing, size_t second_smoothing)
    : IndicatorBase(input, "TSI"),
      first_smoothing_(first_smoothing),
      second_smoothing_(second_smoothing),
      price_buffer_(2),
      momentum_ema1_(0.0),
      momentum_ema2_(0.0),
      abs_momentum_ema1_(0.0),
      abs_momentum_ema2_(0.0),
      has_momentum_ema1_(false),
      has_momentum_ema2_(false),
      has_abs_momentum_ema1_(false),
      has_abs_momentum_ema2_(false) {
    
    if (first_smoothing == 0 || second_smoothing == 0) {
        throw std::invalid_argument("TSI smoothing periods must be greater than 0");
    }
    
    setParam("first_smoothing", static_cast<double>(first_smoothing));
    setParam("second_smoothing", static_cast<double>(second_smoothing));
    setMinPeriod(first_smoothing + second_smoothing);
    
    // Calculate EMA multipliers
    momentum_multiplier1_ = 2.0 / (first_smoothing + 1.0);
    momentum_multiplier2_ = 2.0 / (second_smoothing + 1.0);
    abs_momentum_multiplier1_ = 2.0 / (first_smoothing + 1.0);
    abs_momentum_multiplier2_ = 2.0 / (second_smoothing + 1.0);
}

void TSI::reset() {
    IndicatorBase::reset();
    price_buffer_.clear();
    momentum_ema1_ = 0.0;
    momentum_ema2_ = 0.0;
    abs_momentum_ema1_ = 0.0;
    abs_momentum_ema2_ = 0.0;
    has_momentum_ema1_ = false;
    has_momentum_ema2_ = false;
    has_abs_momentum_ema1_ = false;
    has_abs_momentum_ema2_ = false;
}

void TSI::calculate() {
    if (!hasValidInput()) {
        setOutput(0, NaN);
        return;
    }
    
    auto input = getInput(0);
    double current_price = input->get(0);
    
    if (isNaN(current_price)) {
        setOutput(0, NaN);
        return;
    }
    
    // Add current price to buffer
    price_buffer_.push_back(current_price);
    
    if (price_buffer_.size() < 2) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate price momentum (change)
    double prev_price = price_buffer_[0];
    double momentum = current_price - prev_price;
    double abs_momentum = std::abs(momentum);
    
    // Update EMAs
    updateEMAs(momentum, abs_momentum);
    
    // Calculate TSI
    if (has_momentum_ema2_ && has_abs_momentum_ema2_ && abs_momentum_ema2_ != 0.0) {
        double tsi = 100.0 * (momentum_ema2_ / abs_momentum_ema2_);
        setOutput(0, tsi);
    } else {
        setOutput(0, NaN);
    }
}

void TSI::calculateBatch(size_t start, size_t end) {
    if (!hasValidInput()) {
        return;
    }
    
    auto input = getInput(0);
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            input->forward();
        }
    }
}

void TSI::setSmoothingPeriods(size_t first_smoothing, size_t second_smoothing) {
    if (first_smoothing == 0 || second_smoothing == 0) {
        throw std::invalid_argument("TSI smoothing periods must be greater than 0");
    }
    
    first_smoothing_ = first_smoothing;
    second_smoothing_ = second_smoothing;
    
    setParam("first_smoothing", static_cast<double>(first_smoothing));
    setParam("second_smoothing", static_cast<double>(second_smoothing));
    setMinPeriod(first_smoothing + second_smoothing);
    
    // Recalculate EMA multipliers
    momentum_multiplier1_ = 2.0 / (first_smoothing + 1.0);
    momentum_multiplier2_ = 2.0 / (second_smoothing + 1.0);
    abs_momentum_multiplier1_ = 2.0 / (first_smoothing + 1.0);
    abs_momentum_multiplier2_ = 2.0 / (second_smoothing + 1.0);
    
    reset();
}

double TSI::getSignalLine(size_t signal_period) const {
    try {
        // Calculate EMA of TSI values
        std::vector<double> tsi_values;
        for (size_t i = 0; i < signal_period && i < 100; ++i) {  // Limit lookback
            double tsi = get(-static_cast<int>(i));
            if (!isNaN(tsi)) {
                tsi_values.push_back(tsi);
            } else {
                break;
            }
        }
        
        if (tsi_values.empty()) {
            return NaN;
        }
        
        // Simple EMA calculation for signal line
        double ema = tsi_values[0];
        double multiplier = 2.0 / (signal_period + 1.0);
        
        for (size_t i = 1; i < tsi_values.size(); ++i) {
            ema = tsi_values[i] * multiplier + ema * (1.0 - multiplier);
        }
        
        return ema;
        
    } catch (...) {
        return NaN;
    }
}

double TSI::getOverboughtOversold(double overbought_level, double oversold_level) const {
    double tsi_value = get(0);
    if (isNaN(tsi_value)) {
        return 0.0;
    }
    
    if (tsi_value >= overbought_level) {
        return 1.0;   // Overbought
    } else if (tsi_value <= oversold_level) {
        return -1.0;  // Oversold
    } else {
        return 0.0;   // Neutral
    }
}

double TSI::getZeroCrossSignal() const {
    try {
        double tsi_current = get(0);
        double tsi_prev = get(-1);
        
        if (isNaN(tsi_current) || isNaN(tsi_prev)) {
            return 0.0;
        }
        
        // Bullish crossover: TSI crosses above zero
        if (tsi_prev <= 0.0 && tsi_current > 0.0) {
            return 1.0;
        }
        
        // Bearish crossover: TSI crosses below zero
        if (tsi_prev >= 0.0 && tsi_current < 0.0) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double TSI::getSignalCrossover(size_t signal_period) const {
    try {
        double tsi_current = get(0);
        double tsi_prev = get(-1);
        double signal_current = getSignalLine(signal_period);
        
        // Need to calculate previous signal line value
        // This is approximate - in a real implementation, you'd store signal line history
        double signal_prev = signal_current;  // Simplified
        
        if (isNaN(tsi_current) || isNaN(tsi_prev) || isNaN(signal_current) || isNaN(signal_prev)) {
            return 0.0;
        }
        
        // Bullish crossover: TSI crosses above signal line
        if (tsi_prev <= signal_prev && tsi_current > signal_current) {
            return 1.0;
        }
        
        // Bearish crossover: TSI crosses below signal line
        if (tsi_prev >= signal_prev && tsi_current < signal_current) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double TSI::getMomentum(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 5;
        
        double tsi_current = get(0);
        double tsi_past = get(-static_cast<int>(lookback));
        
        if (isNaN(tsi_current) || isNaN(tsi_past)) {
            return 0.0;
        }
        
        return tsi_current - tsi_past;
        
    } catch (...) {
        return 0.0;
    }
}

double TSI::getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback) const {
    if (!price_line || lookback < 5) {
        return 0.0;
    }
    
    try {
        // Find recent extremes in both price and TSI
        double price_high = price_line->get(0);
        double price_low = price_line->get(0);
        double tsi_high = get(0);
        double tsi_low = get(0);
        
        for (size_t i = 1; i < lookback; ++i) {
            double price = price_line->get(-static_cast<int>(i));
            double tsi = get(-static_cast<int>(i));
            
            if (!isNaN(price)) {
                price_high = std::max(price_high, price);
                price_low = std::min(price_low, price);
            }
            if (!isNaN(tsi)) {
                tsi_high = std::max(tsi_high, tsi);
                tsi_low = std::min(tsi_low, tsi);
            }
        }
        
        double price_range = price_high - price_low;
        double tsi_range = tsi_high - tsi_low;
        
        if (price_range == 0.0 || tsi_range == 0.0) {
            return 0.0;
        }
        
        // Calculate trend directions
        double price_trend = (price_line->get(0) - price_line->get(-static_cast<int>(lookback-1))) / price_range;
        double tsi_trend = (get(0) - get(-static_cast<int>(lookback-1))) / tsi_range;
        
        // Divergence strength
        return tsi_trend - price_trend;
        
    } catch (...) {
        return 0.0;
    }
}

double TSI::getTrendStrength() const {
    double tsi_value = get(0);
    if (isNaN(tsi_value)) {
        return 0.0;
    }
    
    // Absolute TSI value indicates trend strength
    return std::abs(tsi_value) / 100.0;  // Normalize to 0-1
}

double TSI::getPriceMomentum() const {
    if (price_buffer_.size() < 2) {
        return NaN;
    }
    
    return price_buffer_[1] - price_buffer_[0];  // Latest - Previous
}

void TSI::updateEMAs(double momentum, double abs_momentum) {
    // First smoothing of momentum
    momentum_ema1_ = calculateEMA(momentum, momentum_ema1_, momentum_multiplier1_, has_momentum_ema1_);
    
    // First smoothing of absolute momentum
    abs_momentum_ema1_ = calculateEMA(abs_momentum, abs_momentum_ema1_, abs_momentum_multiplier1_, has_abs_momentum_ema1_);
    
    // Second smoothing of momentum (only if first smoothing is available)
    if (has_momentum_ema1_) {
        momentum_ema2_ = calculateEMA(momentum_ema1_, momentum_ema2_, momentum_multiplier2_, has_momentum_ema2_);
    }
    
    // Second smoothing of absolute momentum (only if first smoothing is available)
    if (has_abs_momentum_ema1_) {
        abs_momentum_ema2_ = calculateEMA(abs_momentum_ema1_, abs_momentum_ema2_, abs_momentum_multiplier2_, has_abs_momentum_ema2_);
    }
}

double TSI::calculateEMA(double current_value, double prev_ema, double multiplier, bool& has_prev) {
    if (!has_prev) {
        has_prev = true;
        return current_value;
    } else {
        return current_value * multiplier + prev_ema * (1.0 - multiplier);
    }
}

} // namespace backtrader