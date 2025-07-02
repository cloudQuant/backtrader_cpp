#include "indicators/Stochastic.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace backtrader {

Stochastic::Stochastic(std::shared_ptr<LineRoot> high_input,
                      std::shared_ptr<LineRoot> low_input,
                      std::shared_ptr<LineRoot> close_input,
                      size_t k_period, size_t k_slow, size_t d_period)
    : IndicatorBase(high_input, "Stochastic"),
      k_period_(k_period),
      k_slow_(k_slow),
      d_period_(d_period),
      high_buffer_(k_period),
      low_buffer_(k_period),
      close_buffer_(k_period),
      k_values_(std::max(k_slow, d_period)) {
    
    if (k_period == 0 || k_slow == 0 || d_period == 0) {
        throw std::invalid_argument("Stochastic periods must be greater than 0");
    }
    
    setInputs(high_input, low_input, close_input);
    setParam("k_period", static_cast<double>(k_period));
    setParam("k_slow", static_cast<double>(k_slow));
    setParam("d_period", static_cast<double>(d_period));
    setMinPeriod(k_period + k_slow + d_period - 2);
    
    // Create multiple output lines: %K and %D
    addOutputLine();  // %D line (index 1)
}

void Stochastic::setInputs(std::shared_ptr<LineRoot> high_input,
                          std::shared_ptr<LineRoot> low_input,
                          std::shared_ptr<LineRoot> close_input) {
    if (!high_input || !low_input || !close_input) {
        throw std::invalid_argument("Stochastic requires valid high, low, and close inputs");
    }
    
    // Store additional inputs (base class stores first input)
    addInput(low_input);
    addInput(close_input);
}

void Stochastic::reset() {
    IndicatorBase::reset();
    high_buffer_.clear();
    low_buffer_.clear();
    close_buffer_.clear();
    k_values_.clear();
}

void Stochastic::calculate() {
    if (inputs_.size() < 3) {
        setOutput(0, NaN);  // %K
        setOutput(1, NaN);  // %D
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    auto close_input = getInput(2);
    
    if (!high_input || !low_input || !close_input) {
        setOutput(0, NaN);
        setOutput(1, NaN);
        return;
    }
    
    double current_high = high_input->get(0);
    double current_low = low_input->get(0);
    double current_close = close_input->get(0);
    
    if (isNaN(current_high) || isNaN(current_low) || isNaN(current_close)) {
        setOutput(0, NaN);
        setOutput(1, NaN);
        return;
    }
    
    // Add current values to buffers
    high_buffer_.push_back(current_high);
    low_buffer_.push_back(current_low);
    close_buffer_.push_back(current_close);
    
    if (high_buffer_.size() < k_period_) {
        setOutput(0, NaN);
        setOutput(1, NaN);
        return;
    }
    
    // Find highest high and lowest low over k_period
    double highest_high = *std::max_element(high_buffer_.begin(), high_buffer_.end());
    double lowest_low = *std::min_element(low_buffer_.begin(), low_buffer_.end());
    
    // Calculate raw %K
    double raw_k = calculateRawK(current_close, highest_high, lowest_low);
    k_values_.push_back(raw_k);
    
    // Calculate smoothed %K (slow stochastic)
    double smoothed_k = NaN;
    if (k_values_.size() >= k_slow_) {
        smoothed_k = calculateSMA(k_values_, k_slow_);
    }
    
    // Calculate %D (SMA of %K)
    double d_value = NaN;
    if (k_values_.size() >= d_period_) {
        d_value = calculateSMA(k_values_, d_period_);
    }
    
    setOutput(0, smoothed_k);  // %K line
    setOutput(1, d_value);     // %D line
}

void Stochastic::calculateBatch(size_t start, size_t end) {
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

double Stochastic::getK() const {
    return get(0);  // %K line
}

double Stochastic::getD() const {
    return getOutput(1)->get(0);  // %D line
}

double Stochastic::getOverboughtOversold(double overbought_level, double oversold_level) const {
    double k_value = getK();
    double d_value = getD();
    
    if (isNaN(k_value) || isNaN(d_value)) {
        return 0.0;
    }
    
    // Both %K and %D must be in the zone for confirmation
    if (k_value >= overbought_level && d_value >= overbought_level) {
        return 1.0;   // Overbought
    } else if (k_value <= oversold_level && d_value <= oversold_level) {
        return -1.0;  // Oversold
    }
    
    return 0.0;  // Neutral
}

double Stochastic::getCrossoverSignal() const {
    try {
        double k_current = getK();
        double d_current = getD();
        double k_prev = get(-1);
        double d_prev = getOutput(1)->get(-1);
        
        if (isNaN(k_current) || isNaN(d_current) || isNaN(k_prev) || isNaN(d_prev)) {
            return 0.0;
        }
        
        // Bullish crossover: %K crosses above %D
        if (k_prev <= d_prev && k_current > d_current) {
            return 1.0;
        }
        
        // Bearish crossover: %K crosses below %D
        if (k_prev >= d_prev && k_current < d_current) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double Stochastic::calculateRawK(double close, double highest_high, double lowest_low) const {
    if (highest_high == lowest_low) {
        return 50.0;  // Neutral when no range
    }
    
    return ((close - lowest_low) / (highest_high - lowest_low)) * 100.0;
}

double Stochastic::calculateSMA(const CircularBuffer<double>& buffer, size_t period) const {
    if (buffer.size() < period) {
        return NaN;
    }
    
    double sum = 0.0;
    size_t count = 0;
    
    // Calculate SMA of last 'period' values
    for (size_t i = buffer.size() - period; i < buffer.size(); ++i) {
        if (!isNaN(buffer[i])) {
            sum += buffer[i];
            count++;
        }
    }
    
    return (count > 0) ? sum / count : NaN;
}

} // namespace backtrader