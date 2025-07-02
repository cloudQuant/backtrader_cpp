#include "indicators/Aroon.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>

namespace backtrader {

Aroon::Aroon(std::shared_ptr<LineRoot> high_input,
             std::shared_ptr<LineRoot> low_input,
             size_t period)
    : IndicatorBase(high_input, "Aroon"),
      period_(period),
      high_buffer_(period),
      low_buffer_(period) {
    
    if (period == 0) {
        throw std::invalid_argument("Aroon period must be greater than 0");
    }
    
    setInputs(high_input, low_input);
    setParam("period", static_cast<double>(period));
    setMinPeriod(period);
    
    // Create additional output lines for AroonUp and AroonDown
    addOutputLine();  // AroonDown (index 1)
    addOutputLine();  // AroonOscillator (index 2)
}

void Aroon::setInputs(std::shared_ptr<LineRoot> high_input,
                      std::shared_ptr<LineRoot> low_input) {
    if (!high_input || !low_input) {
        throw std::invalid_argument("Aroon requires valid high and low inputs");
    }
    
    addInput(low_input);
}

void Aroon::reset() {
    IndicatorBase::reset();
    high_buffer_.clear();
    low_buffer_.clear();
}

void Aroon::calculate() {
    if (inputs_.size() < 2) {
        setOutput(0, NaN);  // AroonUp
        setOutput(1, NaN);  // AroonDown
        setOutput(2, NaN);  // AroonOscillator
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    
    if (!high_input || !low_input) {
        setOutput(0, NaN);
        setOutput(1, NaN);
        setOutput(2, NaN);
        return;
    }
    
    double current_high = high_input->get(0);
    double current_low = low_input->get(0);
    
    if (isNaN(current_high) || isNaN(current_low)) {
        setOutput(0, NaN);
        setOutput(1, NaN);
        setOutput(2, NaN);
        return;
    }
    
    high_buffer_.push_back(current_high);
    low_buffer_.push_back(current_low);
    
    if (high_buffer_.size() < period_) {
        setOutput(0, NaN);
        setOutput(1, NaN);
        setOutput(2, NaN);
        return;
    }
    
    auto [high_pos, low_pos] = findHighLowPositions();
    
    // Calculate Aroon Up and Aroon Down
    double aroon_up = ((static_cast<double>(period_) - high_pos) / period_) * 100.0;
    double aroon_down = ((static_cast<double>(period_) - low_pos) / period_) * 100.0;
    double aroon_oscillator = aroon_up - aroon_down;
    
    setOutput(0, aroon_up);
    setOutput(1, aroon_down);
    setOutput(2, aroon_oscillator);
}

void Aroon::calculateBatch(size_t start, size_t end) {
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

void Aroon::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("Aroon period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period);
    
    high_buffer_ = CircularBuffer<double>(period);
    low_buffer_ = CircularBuffer<double>(period);
    reset();
}

double Aroon::getAroonUp() const {
    return get(0);
}

double Aroon::getAroonDown() const {
    return getOutput(1)->get(0);
}

double Aroon::getAroonOscillator() const {
    return getOutput(2)->get(0);
}

double Aroon::getTrendSignal() const {
    double aroon_up = getAroonUp();
    double aroon_down = getAroonDown();
    
    if (isNaN(aroon_up) || isNaN(aroon_down)) {
        return 0.0;
    }
    
    if (aroon_up > 70.0 && aroon_down < 30.0) {
        return 1.0;   // Strong uptrend
    } else if (aroon_down > 70.0 && aroon_up < 30.0) {
        return -1.0;  // Strong downtrend
    } else {
        return 0.0;   // Sideways/unclear
    }
}

double Aroon::getCrossoverSignal() const {
    try {
        double aroon_up_current = getAroonUp();
        double aroon_down_current = getAroonDown();
        double aroon_up_prev = getOutput(0)->get(-1);
        double aroon_down_prev = getOutput(1)->get(-1);
        
        if (isNaN(aroon_up_current) || isNaN(aroon_down_current) ||
            isNaN(aroon_up_prev) || isNaN(aroon_down_prev)) {
            return 0.0;
        }
        
        // Bullish crossover: AroonUp crosses above AroonDown
        if (aroon_up_prev <= aroon_down_prev && aroon_up_current > aroon_down_current) {
            return 1.0;
        }
        
        // Bearish crossover: AroonDown crosses above AroonUp
        if (aroon_down_prev <= aroon_up_prev && aroon_down_current > aroon_up_current) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double Aroon::getTrendStrength() const {
    double aroon_oscillator = getAroonOscillator();
    if (isNaN(aroon_oscillator)) {
        return 0.0;
    }
    
    return std::abs(aroon_oscillator) / 100.0;
}

std::pair<int, int> Aroon::findHighLowPositions() const {
    if (high_buffer_.size() != period_ || low_buffer_.size() != period_) {
        return {0, 0};
    }
    
    // Find position of highest high (0 = most recent)
    double highest = high_buffer_[0];
    int high_pos = 0;
    
    for (size_t i = 1; i < period_; ++i) {
        if (high_buffer_[i] >= highest) {
            highest = high_buffer_[i];
            high_pos = static_cast<int>(i);
        }
    }
    
    // Find position of lowest low (0 = most recent)
    double lowest = low_buffer_[0];
    int low_pos = 0;
    
    for (size_t i = 1; i < period_; ++i) {
        if (low_buffer_[i] <= lowest) {
            lowest = low_buffer_[i];
            low_pos = static_cast<int>(i);
        }
    }
    
    return {high_pos, low_pos};
}

} // namespace backtrader