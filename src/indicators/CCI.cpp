#include "indicators/CCI.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace backtrader {

CCI::CCI(std::shared_ptr<LineRoot> high_input,
         std::shared_ptr<LineRoot> low_input,
         std::shared_ptr<LineRoot> close_input,
         size_t period,
         double factor)
    : IndicatorBase(close_input, "CCI"),
      period_(period),
      factor_(factor),
      high_line_(high_input),
      low_line_(low_input),
      close_line_(close_input),
      tp_buffer_(period),
      tp_sum_(0.0) {
    
    if (period == 0) {
        throw std::invalid_argument("CCI period must be greater than 0");
    }
    
    if (!high_input || !low_input || !close_input) {
        throw std::invalid_argument("High, low, and close price lines are required for CCI");
    }
    
    setParam("period", static_cast<double>(period));
    setParam("factor", factor);
    setMinPeriod(period);
}

void CCI::reset() {
    IndicatorBase::reset();
    tp_buffer_.clear();
    tp_sum_ = 0.0;
}

void CCI::calculate() {
    if (!hasValidInput() || !high_line_ || !low_line_) {
        setOutput(0, NaN);
        return;
    }
    
    double current_high = high_line_->get(0);
    double current_low = low_line_->get(0);
    double current_close = close_line_->get(0);
    
    if (isNaN(current_high) || isNaN(current_low) || isNaN(current_close)) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate Typical Price (TP)
    double typical_price = (current_high + current_low + current_close) / 3.0;
    
    // Update TP buffer and sum
    tp_buffer_.push_back(typical_price);
    tp_sum_ += typical_price;
    
    if (tp_buffer_.size() > period_) {
        tp_sum_ -= tp_buffer_.front();
        tp_buffer_.pop_front();
    }
    
    if (tp_buffer_.size() < period_) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate Simple Moving Average of TP
    double sma_tp = tp_sum_ / period_;
    
    // Calculate Mean Deviation
    double mean_deviation = calculateMeanDeviation(sma_tp);
    
    if (mean_deviation == 0.0) {
        setOutput(0, 0.0);
        return;
    }
    
    // Calculate CCI
    double cci = (typical_price - sma_tp) / (factor_ * mean_deviation);
    setOutput(0, cci);
}

void CCI::calculateBatch(size_t start, size_t end) {
    if (!hasValidInput() || !high_line_ || !low_line_) {
        return;
    }
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
}

void CCI::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("CCI period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period);
    
    // Resize buffer and reset
    tp_buffer_ = CircularBuffer<double>(period);
    tp_sum_ = 0.0;
    reset();
}

void CCI::setFactor(double factor) {
    factor_ = factor;
    setParam("factor", factor);
}

double CCI::getOverboughtOversoldStatus(double overbought_level, double oversold_level) const {
    double cci_value = get(0);
    if (isNaN(cci_value)) {
        return 0.0;
    }
    
    if (cci_value >= overbought_level) {
        return 1.0;  // Overbought
    } else if (cci_value <= oversold_level) {
        return -1.0; // Oversold
    } else {
        return 0.0;  // Neutral
    }
}

double CCI::getTrendSignal() const {
    try {
        double cci_current = get(0);
        double cci_prev = get(-1);
        
        if (isNaN(cci_current) || isNaN(cci_prev)) {
            return 0.0;
        }
        
        // Strong bullish: CCI crosses above +100
        if (cci_prev <= 100.0 && cci_current > 100.0) {
            return 1.0;
        }
        
        // Strong bearish: CCI crosses below -100
        if (cci_prev >= -100.0 && cci_current < -100.0) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double CCI::getZeroCrossSignal() const {
    try {
        double cci_current = get(0);
        double cci_prev = get(-1);
        
        if (isNaN(cci_current) || isNaN(cci_prev)) {
            return 0.0;
        }
        
        // Bullish: CCI crosses above 0
        if (cci_prev <= 0.0 && cci_current > 0.0) {
            return 1.0;
        }
        
        // Bearish: CCI crosses below 0
        if (cci_prev >= 0.0 && cci_current < 0.0) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double CCI::getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback) const {
    if (!price_line || lookback < 3) {
        return 0.0;
    }
    
    try {
        // Find recent extremes
        double price_high = price_line->get(0);
        double price_low = price_line->get(0);
        double cci_high = get(0);
        double cci_low = get(0);
        
        for (size_t i = 1; i < lookback; ++i) {
            double price = price_line->get(-static_cast<int>(i));
            double cci = get(-static_cast<int>(i));
            
            if (!isNaN(price)) {
                price_high = std::max(price_high, price);
                price_low = std::min(price_low, price);
            }
            if (!isNaN(cci)) {
                cci_high = std::max(cci_high, cci);
                cci_low = std::min(cci_low, cci);
            }
        }
        
        double price_range = price_high - price_low;
        double cci_range = cci_high - cci_low;
        
        if (price_range == 0.0 || cci_range == 0.0) {
            return 0.0;
        }
        
        // Calculate trend directions
        double price_trend = (price_line->get(0) - price_line->get(-static_cast<int>(lookback-1))) / price_range;
        double cci_trend = (get(0) - get(-static_cast<int>(lookback-1))) / cci_range;
        
        // Divergence strength
        return cci_trend - price_trend;
        
    } catch (...) {
        return 0.0;
    }
}

double CCI::getMomentum(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 1;
        
        double cci_current = get(0);
        double cci_past = get(-static_cast<int>(lookback));
        
        if (isNaN(cci_current) || isNaN(cci_past)) {
            return 0.0;
        }
        
        return cci_current - cci_past;
        
    } catch (...) {
        return 0.0;
    }
}

double CCI::getVolatility(size_t lookback) const {
    try {
        if (lookback < 2) lookback = 10;
        
        std::vector<double> values;
        for (size_t i = 0; i < lookback; ++i) {
            double cci = get(-static_cast<int>(i));
            if (!isNaN(cci)) {
                values.push_back(cci);
            }
        }
        
        if (values.size() < 2) {
            return 0.0;
        }
        
        // Calculate standard deviation
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

double CCI::getStrength() const {
    double cci_value = get(0);
    if (isNaN(cci_value)) {
        return 0.0;
    }
    
    // Normalize CCI strength to 0-100 scale
    // CCI typically ranges from -300 to +300, but can go beyond
    double strength = std::abs(cci_value);
    
    if (strength >= 200.0) {
        return 100.0;  // Very strong
    } else if (strength >= 100.0) {
        return 50.0 + (strength - 100.0) / 2.0;  // Strong to very strong
    } else {
        return strength / 2.0;  // Weak to moderate
    }
}

double CCI::calculateMeanDeviation(double sma_tp) const {
    if (tp_buffer_.size() < period_) {
        return 0.0;
    }
    
    double sum_deviation = 0.0;
    for (size_t i = 0; i < tp_buffer_.size(); ++i) {
        sum_deviation += std::abs(tp_buffer_[i] - sma_tp);
    }
    
    return sum_deviation / period_;
}

} // namespace backtrader