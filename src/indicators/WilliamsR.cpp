#include "indicators/WilliamsR.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace backtrader {

WilliamsR::WilliamsR(std::shared_ptr<LineRoot> high_input,
                     std::shared_ptr<LineRoot> low_input,
                     std::shared_ptr<LineRoot> close_input,
                     size_t period)
    : IndicatorBase(close_input, "WilliamsR"),
      period_(period),
      high_line_(high_input),
      low_line_(low_input),
      close_line_(close_input),
      high_buffer_(period),
      low_buffer_(period) {
    
    if (period == 0) {
        throw std::invalid_argument("Williams %R period must be greater than 0");
    }
    
    if (!high_input || !low_input || !close_input) {
        throw std::invalid_argument("High, low, and close price lines are required for Williams %R");
    }
    
    setParam("period", static_cast<double>(period));
    setMinPeriod(period);
}

void WilliamsR::reset() {
    IndicatorBase::reset();
    high_buffer_.clear();
    low_buffer_.clear();
}

void WilliamsR::calculate() {
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
    
    // Update buffers
    high_buffer_.push_back(current_high);
    low_buffer_.push_back(current_low);
    
    if (high_buffer_.size() < period_) {
        setOutput(0, NaN);
        return;
    }
    
    // Find highest high and lowest low over the period
    double highest_high = *std::max_element(high_buffer_.begin(), high_buffer_.end());
    double lowest_low = *std::min_element(low_buffer_.begin(), low_buffer_.end());
    
    // Calculate Williams %R
    double williams_r = NaN;
    if (highest_high != lowest_low) {
        williams_r = ((highest_high - current_close) / (highest_high - lowest_low)) * -100.0;
    }
    
    setOutput(0, williams_r);
}

void WilliamsR::calculateBatch(size_t start, size_t end) {
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

void WilliamsR::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("Williams %R period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period);
    
    // Resize buffers and reset
    high_buffer_ = CircularBuffer<double>(period);
    low_buffer_ = CircularBuffer<double>(period);
    reset();
}

double WilliamsR::getOverboughtOversoldStatus(double overbought_level, double oversold_level) const {
    double wr_value = get(0);
    if (isNaN(wr_value)) {
        return 0.0;
    }
    
    // Williams %R ranges from -100 to 0
    // Overbought: -20 and above (closer to 0)
    // Oversold: -80 and below (closer to -100)
    if (wr_value >= overbought_level) {
        return 1.0;  // Overbought
    } else if (wr_value <= oversold_level) {
        return -1.0; // Oversold
    } else {
        return 0.0;  // Neutral
    }
}

double WilliamsR::getReversalSignal() const {
    try {
        double wr_current = get(0);
        double wr_prev = get(-1);
        double wr_prev2 = get(-2);
        
        if (isNaN(wr_current) || isNaN(wr_prev) || isNaN(wr_prev2)) {
            return 0.0;
        }
        
        // Bullish reversal: Williams %R moves from oversold (-80 or below) to above -80
        if (wr_prev <= -80.0 && wr_current > -80.0) {
            return 1.0;  // Bullish reversal
        }
        
        // Bearish reversal: Williams %R moves from overbought (-20 or above) to below -20
        if (wr_prev >= -20.0 && wr_current < -20.0) {
            return -1.0; // Bearish reversal
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double WilliamsR::getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback) const {
    if (!price_line || lookback < 3) {
        return 0.0;
    }
    
    try {
        // Find recent extremes
        double price_high = price_line->get(0);
        double price_low = price_line->get(0);
        double wr_high = get(0);
        double wr_low = get(0);
        
        for (size_t i = 1; i < lookback; ++i) {
            double price = price_line->get(-static_cast<int>(i));
            double wr = get(-static_cast<int>(i));
            
            if (!isNaN(price)) {
                price_high = std::max(price_high, price);
                price_low = std::min(price_low, price);
            }
            if (!isNaN(wr)) {
                wr_high = std::max(wr_high, wr);  // Remember: Williams %R is negative
                wr_low = std::min(wr_low, wr);
            }
        }
        
        double price_range = price_high - price_low;
        double wr_range = wr_high - wr_low;
        
        if (price_range == 0.0 || wr_range == 0.0) {
            return 0.0;
        }
        
        // Calculate trend directions
        double price_trend = (price_line->get(0) - price_line->get(-static_cast<int>(lookback-1))) / price_range;
        double wr_trend = (get(0) - get(-static_cast<int>(lookback-1))) / wr_range;
        
        // Divergence: price and Williams %R moving in opposite directions
        return wr_trend - price_trend;
        
    } catch (...) {
        return 0.0;
    }
}

double WilliamsR::getMomentum(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 1;
        
        double wr_current = get(0);
        double wr_past = get(-static_cast<int>(lookback));
        
        if (isNaN(wr_current) || isNaN(wr_past)) {
            return 0.0;
        }
        
        return wr_current - wr_past;
        
    } catch (...) {
        return 0.0;
    }
}

double WilliamsR::getVolatility(size_t lookback) const {
    try {
        if (lookback < 2) lookback = 10;
        
        std::vector<double> values;
        for (size_t i = 0; i < lookback; ++i) {
            double wr = get(-static_cast<int>(i));
            if (!isNaN(wr)) {
                values.push_back(wr);
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

double WilliamsR::getStrength() const {
    double wr_value = get(0);
    if (isNaN(wr_value)) {
        return 0.0;
    }
    
    // Convert Williams %R (-100 to 0) to strength (0 to 100)
    // Values closer to 0 indicate stronger bullish momentum
    // Values closer to -100 indicate stronger bearish momentum
    return 100.0 + wr_value;  // This converts -100,0 range to 0,100 range
}

} // namespace backtrader