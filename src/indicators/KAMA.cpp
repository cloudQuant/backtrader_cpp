#include "indicators/KAMA.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace backtrader {

KAMA::KAMA(std::shared_ptr<LineRoot> input, size_t period, size_t fast_sc, size_t slow_sc)
    : IndicatorBase(input, "KAMA"),
      period_(period),
      fast_sc_(fast_sc),
      slow_sc_(slow_sc),
      price_buffer_(period + 1),
      prev_kama_(NaN),
      has_prev_kama_(false) {
    
    if (period == 0) {
        throw std::invalid_argument("KAMA period must be greater than 0");
    }
    
    if (fast_sc >= slow_sc) {
        throw std::invalid_argument("Fast SC must be less than Slow SC");
    }
    
    setParam("period", static_cast<double>(period));
    setParam("fast_sc", static_cast<double>(fast_sc));
    setParam("slow_sc", static_cast<double>(slow_sc));
    setMinPeriod(period + 1);
    
    // Pre-calculate smoothing constants
    fast_sc_calc_ = 2.0 / (fast_sc + 1.0);
    slow_sc_calc_ = 2.0 / (slow_sc + 1.0);
}

void KAMA::reset() {
    IndicatorBase::reset();
    price_buffer_.clear();
    prev_kama_ = NaN;
    has_prev_kama_ = false;
}

void KAMA::calculate() {
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
    
    if (price_buffer_.size() <= period_) {
        if (price_buffer_.size() == 1) {
            // First value: initialize KAMA with current price
            prev_kama_ = current_price;
            has_prev_kama_ = true;
            setOutput(0, current_price);
        } else {
            setOutput(0, NaN);
        }
        return;
    }
    
    // Calculate Direction (net price change over period)
    double direction = std::abs(current_price - price_buffer_[0]);
    
    // Calculate Volatility (sum of absolute price changes)
    double volatility = 0.0;
    for (size_t i = 1; i < price_buffer_.size(); ++i) {
        volatility += std::abs(price_buffer_[i] - price_buffer_[i-1]);
    }
    
    // Calculate Efficiency Ratio
    double er = (volatility != 0.0) ? direction / volatility : 0.0;
    
    // Calculate Smoothing Constant
    double sc_diff = fast_sc_calc_ - slow_sc_calc_;
    double sc = slow_sc_calc_ + er * sc_diff;
    sc = sc * sc;  // Square the smoothing constant
    
    // Calculate KAMA
    double kama = prev_kama_ + sc * (current_price - prev_kama_);
    
    prev_kama_ = kama;
    setOutput(0, kama);
}

void KAMA::calculateBatch(size_t start, size_t end) {
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

void KAMA::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("KAMA period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period + 1);
    
    // Resize buffer and reset
    price_buffer_ = CircularBuffer<double>(period + 1);
    reset();
}

void KAMA::setSmoothing(size_t fast_sc, size_t slow_sc) {
    if (fast_sc >= slow_sc) {
        throw std::invalid_argument("Fast SC must be less than Slow SC");
    }
    
    fast_sc_ = fast_sc;
    slow_sc_ = slow_sc;
    setParam("fast_sc", static_cast<double>(fast_sc));
    setParam("slow_sc", static_cast<double>(slow_sc));
    
    // Recalculate smoothing constants
    fast_sc_calc_ = 2.0 / (fast_sc + 1.0);
    slow_sc_calc_ = 2.0 / (slow_sc + 1.0);
}

double KAMA::getEfficiencyRatio() const {
    if (price_buffer_.size() <= period_) {
        return NaN;
    }
    
    // Calculate current efficiency ratio
    double direction = std::abs(price_buffer_.back() - price_buffer_[0]);
    
    double volatility = 0.0;
    for (size_t i = 1; i < price_buffer_.size(); ++i) {
        volatility += std::abs(price_buffer_[i] - price_buffer_[i-1]);
    }
    
    return (volatility != 0.0) ? direction / volatility : 0.0;
}

double KAMA::getSmoothingConstant() const {
    double er = getEfficiencyRatio();
    if (isNaN(er)) {
        return NaN;
    }
    
    double sc_diff = fast_sc_calc_ - slow_sc_calc_;
    double sc = slow_sc_calc_ + er * sc_diff;
    return sc * sc;
}

double KAMA::getTrendSignal(double threshold) const {
    try {
        double kama_current = get(0);
        double kama_prev = get(-1);
        
        if (isNaN(kama_current) || isNaN(kama_prev)) {
            return 0.0;
        }
        
        double change = (kama_current - kama_prev) / kama_prev;
        
        if (change > threshold) {
            return 1.0;  // Strong uptrend
        } else if (change < -threshold) {
            return -1.0; // Strong downtrend
        } else {
            return 0.0;  // Sideways
        }
        
    } catch (...) {
        return 0.0;
    }
}

double KAMA::getVolatility(size_t lookback) const {
    try {
        if (lookback < 2) lookback = 10;
        
        std::vector<double> values;
        for (size_t i = 0; i < lookback; ++i) {
            double kama = get(-static_cast<int>(i));
            if (!isNaN(kama)) {
                values.push_back(kama);
            }
        }
        
        if (values.size() < 2) {
            return 0.0;
        }
        
        // Calculate standard deviation of KAMA values
        double mean = 0.0;
        for (double val : values) {
            mean += val;
        }
        mean /= values.size();
        
        double variance = 0.0;
        for (double val : values) {
            variance += std::pow(val - mean, 2);
        }
        variance /= (values.size() - 1);
        
        return std::sqrt(variance);
        
    } catch (...) {
        return 0.0;
    }
}

double KAMA::getAdaptiveSpeed() const {
    double er = getEfficiencyRatio();
    if (isNaN(er)) {
        return 0.0;
    }
    
    // Return adaptive speed as percentage (0-100%)
    return er * 100.0;
}

double KAMA::getCrossoverSignal(std::shared_ptr<LineRoot> reference_line) const {
    try {
        if (!reference_line) {
            return 0.0;
        }
        
        double kama_current = get(0);
        double kama_prev = get(-1);
        double ref_current = reference_line->get(0);
        double ref_prev = reference_line->get(-1);
        
        if (isNaN(kama_current) || isNaN(kama_prev) || isNaN(ref_current) || isNaN(ref_prev)) {
            return 0.0;
        }
        
        // Bullish crossover: KAMA crosses above reference
        if (kama_prev <= ref_prev && kama_current > ref_current) {
            return 1.0;
        }
        
        // Bearish crossover: KAMA crosses below reference
        if (kama_prev >= ref_prev && kama_current < ref_current) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

} // namespace backtrader