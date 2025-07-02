#include "indicators/WMA.h"
#include "Common.h"
#include <stdexcept>
#include <numeric>

namespace backtrader {

WMA::WMA(std::shared_ptr<LineRoot> input, size_t period)
    : IndicatorBase(input, "WMA"),
      period_(period),
      price_buffer_(period),
      weight_sum_(0.0) {
    
    if (period == 0) {
        throw std::invalid_argument("WMA period must be greater than 0");
    }
    
    setParam("period", static_cast<double>(period));
    setMinPeriod(period);
    
    // Calculate weight sum for normalization
    weight_sum_ = static_cast<double>(period * (period + 1)) / 2.0;
}

void WMA::reset() {
    IndicatorBase::reset();
    price_buffer_.clear();
}

void WMA::calculate() {
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
    
    if (price_buffer_.size() < period_) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate weighted moving average
    double weighted_sum = 0.0;
    for (size_t i = 0; i < price_buffer_.size(); ++i) {
        // Weight increases with recency (newest data has highest weight)
        double weight = static_cast<double>(i + 1);
        weighted_sum += price_buffer_[i] * weight;
    }
    
    double wma = weighted_sum / weight_sum_;
    setOutput(0, wma);
}

void WMA::calculateBatch(size_t start, size_t end) {
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

void WMA::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("WMA period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period);
    
    // Recalculate weight sum
    weight_sum_ = static_cast<double>(period * (period + 1)) / 2.0;
    
    // Resize buffer and reset
    price_buffer_ = CircularBuffer<double>(period);
    reset();
}

double WMA::getSlope(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 1;
        
        double current_wma = get(0);
        double past_wma = get(-static_cast<int>(lookback));
        
        if (isNaN(current_wma) || isNaN(past_wma)) {
            return 0.0;
        }
        
        return (current_wma - past_wma) / lookback;
        
    } catch (...) {
        return 0.0;
    }
}

double WMA::getTrendStrength(size_t lookback) const {
    try {
        if (lookback < 3) lookback = 5;
        
        std::vector<double> values;
        for (size_t i = 0; i < lookback; ++i) {
            double wma = get(-static_cast<int>(i));
            if (!isNaN(wma)) {
                values.push_back(wma);
            }
        }
        
        if (values.size() < 3) {
            return 0.0;
        }
        
        // Calculate linear regression slope
        size_t n = values.size();
        double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
        
        for (size_t i = 0; i < n; ++i) {
            double x = static_cast<double>(i);
            double y = values[n - 1 - i];  // Reverse order for proper time sequence
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
        }
        
        double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
        
        // Normalize slope by average value
        double avg_value = sum_y / n;
        return (avg_value != 0.0) ? (slope / avg_value) * 100.0 : 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double WMA::getCrossoverSignal(std::shared_ptr<LineRoot> reference_line) const {
    try {
        if (!reference_line) {
            return 0.0;
        }
        
        double wma_current = get(0);
        double wma_prev = get(-1);
        double ref_current = reference_line->get(0);
        double ref_prev = reference_line->get(-1);
        
        if (isNaN(wma_current) || isNaN(wma_prev) || isNaN(ref_current) || isNaN(ref_prev)) {
            return 0.0;
        }
        
        // Bullish crossover: WMA crosses above reference
        if (wma_prev <= ref_prev && wma_current > ref_current) {
            return 1.0;
        }
        
        // Bearish crossover: WMA crosses below reference
        if (wma_prev >= ref_prev && wma_current < ref_current) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double WMA::getVolatility(size_t lookback) const {
    try {
        if (lookback < 2) lookback = 10;
        
        std::vector<double> values;
        for (size_t i = 0; i < lookback; ++i) {
            double wma = get(-static_cast<int>(i));
            if (!isNaN(wma)) {
                values.push_back(wma);
            }
        }
        
        if (values.size() < 2) {
            return 0.0;
        }
        
        // Calculate standard deviation of WMA values
        double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        
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

} // namespace backtrader