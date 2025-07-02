#include "indicators/ROC.h"
#include "Common.h"
#include <stdexcept>
#include <cmath>

namespace backtrader {

ROC::ROC(std::shared_ptr<LineRoot> input, size_t period, bool as_percentage)
    : IndicatorBase(input, "ROC"),
      period_(period),
      as_percentage_(as_percentage),
      price_buffer_(period + 1) {
    
    if (period == 0) {
        throw std::invalid_argument("ROC period must be greater than 0");
    }
    
    setParam("period", static_cast<double>(period));
    setParam("as_percentage", as_percentage ? 1.0 : 0.0);
    setMinPeriod(period + 1);
}

void ROC::reset() {
    IndicatorBase::reset();
    price_buffer_.clear();
}

void ROC::calculate() {
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
        setOutput(0, NaN);
        return;
    }
    
    // Get price from period ago
    double past_price = price_buffer_[0];
    
    if (isNaN(past_price) || past_price == 0.0) {
        setOutput(0, NaN);
        return;
    }
    
    double roc;
    if (as_percentage_) {
        // Percentage ROC = ((Current - Past) / Past) * 100
        roc = ((current_price - past_price) / past_price) * 100.0;
    } else {
        // Ratio ROC = Current / Past
        roc = current_price / past_price;
    }
    
    setOutput(0, roc);
}

void ROC::calculateBatch(size_t start, size_t end) {
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

void ROC::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("ROC period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period + 1);
    
    // Resize buffer and reset
    price_buffer_ = CircularBuffer<double>(period + 1);
    reset();
}

void ROC::setAsPercentage(bool as_percentage) {
    as_percentage_ = as_percentage;
    setParam("as_percentage", as_percentage ? 1.0 : 0.0);
}

double ROC::getTrendSignal(double threshold) const {
    double roc_value = get(0);
    if (isNaN(roc_value)) {
        return 0.0;
    }
    
    if (as_percentage_) {
        // For percentage ROC, use percentage threshold
        if (roc_value > threshold) {
            return 1.0;  // Strong positive momentum
        } else if (roc_value < -threshold) {
            return -1.0; // Strong negative momentum
        }
    } else {
        // For ratio ROC, compare with 1.0 + threshold/100
        double upper_threshold = 1.0 + threshold / 100.0;
        double lower_threshold = 1.0 - threshold / 100.0;
        
        if (roc_value > upper_threshold) {
            return 1.0;  // Positive momentum
        } else if (roc_value < lower_threshold) {
            return -1.0; // Negative momentum
        }
    }
    
    return 0.0; // Neutral
}

double ROC::getZeroCrossSignal() const {
    try {
        double roc_current = get(0);
        double roc_prev = get(-1);
        
        if (isNaN(roc_current) || isNaN(roc_prev)) {
            return 0.0;
        }
        
        double zero_line = as_percentage_ ? 0.0 : 1.0;
        
        // Bullish crossover: ROC crosses above zero line
        if (roc_prev <= zero_line && roc_current > zero_line) {
            return 1.0;
        }
        
        // Bearish crossover: ROC crosses below zero line
        if (roc_prev >= zero_line && roc_current < zero_line) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double ROC::getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback) const {
    if (!price_line || lookback < 3) {
        return 0.0;
    }
    
    try {
        // Calculate price momentum over same period
        double current_price = price_line->get(0);
        double past_price = price_line->get(-static_cast<int>(lookback));
        
        if (isNaN(current_price) || isNaN(past_price) || past_price == 0.0) {
            return 0.0;
        }
        
        double price_roc = as_percentage_ ? 
            ((current_price - past_price) / past_price) * 100.0 :
            current_price / past_price;
        
        double indicator_roc = get(0);
        if (isNaN(indicator_roc)) {
            return 0.0;
        }
        
        // Normalize both ROCs for comparison
        double price_direction = 0.0;
        double indicator_direction = 0.0;
        
        if (as_percentage_) {
            price_direction = (price_roc > 0) ? 1.0 : (price_roc < 0) ? -1.0 : 0.0;
            indicator_direction = (indicator_roc > 0) ? 1.0 : (indicator_roc < 0) ? -1.0 : 0.0;
        } else {
            price_direction = (price_roc > 1.0) ? 1.0 : (price_roc < 1.0) ? -1.0 : 0.0;
            indicator_direction = (indicator_roc > 1.0) ? 1.0 : (indicator_roc < 1.0) ? -1.0 : 0.0;
        }
        
        // Divergence occurs when directions are opposite
        if (price_direction != indicator_direction && price_direction != 0.0 && indicator_direction != 0.0) {
            return indicator_direction - price_direction;  // Divergence strength
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double ROC::getAcceleration(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 1;
        
        double roc_current = get(0);
        double roc_past = get(-static_cast<int>(lookback));
        
        if (isNaN(roc_current) || isNaN(roc_past)) {
            return 0.0;
        }
        
        // Acceleration is the rate of change of ROC
        if (as_percentage_) {
            return roc_current - roc_past;
        } else {
            return (roc_past != 0.0) ? (roc_current - roc_past) / roc_past : 0.0;
        }
        
    } catch (...) {
        return 0.0;
    }
}

double ROC::getVolatility(size_t lookback) const {
    try {
        if (lookback < 2) lookback = 10;
        
        std::vector<double> values;
        for (size_t i = 0; i < lookback; ++i) {
            double roc = get(-static_cast<int>(i));
            if (!isNaN(roc)) {
                values.push_back(roc);
            }
        }
        
        if (values.size() < 2) {
            return 0.0;
        }
        
        // Calculate standard deviation
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

double ROC::getStrength() const {
    double roc_value = get(0);
    if (isNaN(roc_value)) {
        return 0.0;
    }
    
    if (as_percentage_) {
        return std::abs(roc_value);
    } else {
        return std::abs(roc_value - 1.0) * 100.0;  // Convert to percentage-like
    }
}

double ROC::getOscillatorPosition(double upper_threshold, double lower_threshold) const {
    double roc_value = get(0);
    if (isNaN(roc_value)) {
        return 0.0;
    }
    
    // Adjust thresholds for ratio mode
    if (!as_percentage_) {
        upper_threshold = 1.0 + upper_threshold / 100.0;
        lower_threshold = 1.0 + lower_threshold / 100.0;
    }
    
    if (roc_value >= upper_threshold) {
        return 1.0;  // Overbought
    } else if (roc_value <= lower_threshold) {
        return -1.0; // Oversold
    } else {
        return 0.0;  // Neutral
    }
}

} // namespace backtrader