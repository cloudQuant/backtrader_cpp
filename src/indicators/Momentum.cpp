#include "indicators/Momentum.h"
#include "Common.h"
#include <stdexcept>
#include <cmath>

namespace backtrader {

Momentum::Momentum(std::shared_ptr<LineRoot> input, size_t period, bool as_percentage)
    : IndicatorBase(input, "Momentum"),
      period_(period),
      as_percentage_(as_percentage),
      price_buffer_(period + 1) {
    
    if (period == 0) {
        throw std::invalid_argument("Momentum period must be greater than 0");
    }
    
    setParam("period", static_cast<double>(period));
    setParam("as_percentage", as_percentage ? 1.0 : 0.0);
    setMinPeriod(period + 1);
}

void Momentum::reset() {
    IndicatorBase::reset();
    price_buffer_.clear();
}

void Momentum::calculate() {
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
    double past_price = price_buffer_[0];  // First element is period+1 ago
    
    if (isNaN(past_price) || past_price == 0.0) {
        setOutput(0, NaN);
        return;
    }
    
    double momentum;
    if (as_percentage_) {
        // Percentage momentum = ((Current - Past) / Past) * 100
        momentum = ((current_price - past_price) / past_price) * 100.0;
    } else {
        // Absolute momentum = Current - Past
        momentum = current_price - past_price;
    }
    
    setOutput(0, momentum);
}

void Momentum::calculateBatch(size_t start, size_t end) {
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

void Momentum::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("Momentum period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period + 1);
    
    // Resize buffer and reset
    price_buffer_ = CircularBuffer<double>(period + 1);
    reset();
}

void Momentum::setAsPercentage(bool as_percentage) {
    as_percentage_ = as_percentage;
    setParam("as_percentage", as_percentage ? 1.0 : 0.0);
}

double Momentum::getTrendSignal(double threshold) const {
    double momentum_value = get(0);
    if (isNaN(momentum_value)) {
        return 0.0;
    }
    
    if (as_percentage_) {
        // For percentage momentum, use percentage threshold
        if (momentum_value > threshold) {
            return 1.0;  // Strong bullish momentum
        } else if (momentum_value < -threshold) {
            return -1.0; // Strong bearish momentum
        }
    } else {
        // For absolute momentum, use absolute threshold
        if (momentum_value > threshold) {
            return 1.0;  // Positive momentum
        } else if (momentum_value < -threshold) {
            return -1.0; // Negative momentum
        }
    }
    
    return 0.0; // Neutral
}

double Momentum::getCrossoverSignal() const {
    try {
        double momentum_current = get(0);
        double momentum_prev = get(-1);
        
        if (isNaN(momentum_current) || isNaN(momentum_prev)) {
            return 0.0;
        }
        
        // Zero line crossover
        if (momentum_prev <= 0.0 && momentum_current > 0.0) {
            return 1.0;  // Bullish crossover
        } else if (momentum_prev >= 0.0 && momentum_current < 0.0) {
            return -1.0; // Bearish crossover
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double Momentum::getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback) const {
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
        
        double price_momentum = as_percentage_ ? 
            ((current_price - past_price) / past_price) * 100.0 :
            current_price - past_price;
        
        double indicator_momentum = get(0);
        if (isNaN(indicator_momentum)) {
            return 0.0;
        }
        
        // Normalize both momentums for comparison
        double price_direction = (price_momentum > 0) ? 1.0 : (price_momentum < 0) ? -1.0 : 0.0;
        double indicator_direction = (indicator_momentum > 0) ? 1.0 : (indicator_momentum < 0) ? -1.0 : 0.0;
        
        // Divergence occurs when directions are opposite
        if (price_direction != indicator_direction && price_direction != 0.0 && indicator_direction != 0.0) {
            return indicator_direction - price_direction;  // Divergence strength
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double Momentum::getAcceleration(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 1;
        
        double momentum_current = get(0);
        double momentum_past = get(-static_cast<int>(lookback));
        
        if (isNaN(momentum_current) || isNaN(momentum_past)) {
            return 0.0;
        }
        
        // Acceleration is the rate of change of momentum
        return momentum_current - momentum_past;
        
    } catch (...) {
        return 0.0;
    }
}

double Momentum::getVolatility(size_t lookback) const {
    try {
        if (lookback < 2) lookback = 10;
        
        std::vector<double> values;
        for (size_t i = 0; i < lookback; ++i) {
            double momentum = get(-static_cast<int>(i));
            if (!isNaN(momentum)) {
                values.push_back(momentum);
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

double Momentum::getStrength() const {
    double momentum_value = get(0);
    if (isNaN(momentum_value)) {
        return 0.0;
    }
    
    // Return absolute momentum as strength indicator
    return std::abs(momentum_value);
}

double Momentum::getRelativeStrength(size_t lookback) const {
    try {
        if (lookback < 2) lookback = 10;
        
        // Calculate average absolute momentum over lookback period
        double sum_abs_momentum = 0.0;
        int count = 0;
        
        for (size_t i = 0; i < lookback; ++i) {
            double momentum = get(-static_cast<int>(i));
            if (!isNaN(momentum)) {
                sum_abs_momentum += std::abs(momentum);
                count++;
            }
        }
        
        if (count == 0) {
            return 0.0;
        }
        
        double avg_abs_momentum = sum_abs_momentum / count;
        double current_abs_momentum = std::abs(get(0));
        
        if (avg_abs_momentum == 0.0) {
            return 0.0;
        }
        
        // Return relative strength as ratio
        return current_abs_momentum / avg_abs_momentum;
        
    } catch (...) {
        return 0.0;
    }
}

} // namespace backtrader