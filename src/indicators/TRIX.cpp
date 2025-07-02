#include "indicators/TRIX.h"
#include "Common.h"
#include <stdexcept>
#include <cmath>

namespace backtrader {

TRIX::TRIX(std::shared_ptr<LineRoot> input, size_t period)
    : IndicatorBase(input, "TRIX"),
      period_(period),
      multiplier_(2.0 / (period + 1.0)),
      ema1_(0.0),
      ema2_(0.0),
      ema3_(0.0),
      prev_ema3_(0.0),
      has_ema1_(false),
      has_ema2_(false),
      has_ema3_(false),
      has_prev_ema3_(false) {
    
    if (period == 0) {
        throw std::invalid_argument("TRIX period must be greater than 0");
    }
    
    setParam("period", static_cast<double>(period));
    setMinPeriod(period * 3 + 1);  // Need time for triple smoothing plus one for rate calculation
}

void TRIX::reset() {
    IndicatorBase::reset();
    ema1_ = 0.0;
    ema2_ = 0.0;
    ema3_ = 0.0;
    prev_ema3_ = 0.0;
    has_ema1_ = false;
    has_ema2_ = false;
    has_ema3_ = false;
    has_prev_ema3_ = false;
}

void TRIX::calculate() {
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
    
    // Calculate first EMA
    ema1_ = calculateEMA(current_price, ema1_, has_ema1_);
    
    if (!has_ema1_) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate second EMA (of first EMA)
    ema2_ = calculateEMA(ema1_, ema2_, has_ema2_);
    
    if (!has_ema2_) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate third EMA (of second EMA)
    ema3_ = calculateEMA(ema2_, ema3_, has_ema3_);
    
    if (!has_ema3_) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate TRIX (rate of change of triple EMA)
    if (has_prev_ema3_ && prev_ema3_ != 0.0) {
        double trix = ((ema3_ - prev_ema3_) / prev_ema3_) * 10000.0;
        setOutput(0, trix);
    } else {
        setOutput(0, NaN);
    }
    
    // Store current EMA3 as previous for next calculation
    prev_ema3_ = ema3_;
    has_prev_ema3_ = true;
}

void TRIX::calculateBatch(size_t start, size_t end) {
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

void TRIX::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("TRIX period must be greater than 0");
    }
    
    period_ = period;
    multiplier_ = 2.0 / (period + 1.0);
    setParam("period", static_cast<double>(period));
    setMinPeriod(period * 3 + 1);
    reset();
}

double TRIX::getSignalLine(size_t signal_period) const {
    try {
        // Calculate EMA of TRIX values over signal_period
        std::vector<double> trix_values;
        for (size_t i = 0; i < signal_period && i < 100; ++i) {
            double trix = get(-static_cast<int>(i));
            if (!isNaN(trix)) {
                trix_values.push_back(trix);
            } else {
                break;
            }
        }
        
        if (trix_values.empty()) {
            return NaN;
        }
        
        // Calculate EMA of TRIX
        double signal_multiplier = 2.0 / (signal_period + 1.0);
        double signal_ema = trix_values[0];
        
        for (size_t i = 1; i < trix_values.size(); ++i) {
            signal_ema = trix_values[i] * signal_multiplier + signal_ema * (1.0 - signal_multiplier);
        }
        
        return signal_ema;
        
    } catch (...) {
        return NaN;
    }
}

double TRIX::getZeroCrossSignal() const {
    try {
        double trix_current = get(0);
        double trix_prev = get(-1);
        
        if (isNaN(trix_current) || isNaN(trix_prev)) {
            return 0.0;
        }
        
        // Bullish crossover: TRIX crosses above zero
        if (trix_prev <= 0.0 && trix_current > 0.0) {
            return 1.0;
        }
        
        // Bearish crossover: TRIX crosses below zero
        if (trix_prev >= 0.0 && trix_current < 0.0) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double TRIX::getSignalCrossover(size_t signal_period) const {
    try {
        double trix_current = get(0);
        double signal_current = getSignalLine(signal_period);
        
        if (isNaN(trix_current) || isNaN(signal_current)) {
            return 0.0;
        }
        
        // For proper crossover detection, we'd need signal line history
        // This is a simplified version
        if (trix_current > signal_current) {
            return 1.0;   // TRIX above signal
        } else if (trix_current < signal_current) {
            return -1.0;  // TRIX below signal
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double TRIX::getMomentum(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 5;
        
        double trix_current = get(0);
        double trix_past = get(-static_cast<int>(lookback));
        
        if (isNaN(trix_current) || isNaN(trix_past)) {
            return 0.0;
        }
        
        return trix_current - trix_past;
        
    } catch (...) {
        return 0.0;
    }
}

double TRIX::getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback) const {
    if (!price_line || lookback < 5) {
        return 0.0;
    }
    
    try {
        // Calculate price trend
        double price_current = price_line->get(0);
        double price_past = price_line->get(-static_cast<int>(lookback));
        
        if (isNaN(price_current) || isNaN(price_past) || price_past == 0.0) {
            return 0.0;
        }
        
        double price_change = (price_current - price_past) / price_past;
        
        // Calculate TRIX trend
        double trix_current = get(0);
        double trix_past = get(-static_cast<int>(lookback));
        
        if (isNaN(trix_current) || isNaN(trix_past)) {
            return 0.0;
        }
        
        double trix_change = trix_current - trix_past;
        
        // Normalize directions for comparison
        double price_direction = (price_change > 0) ? 1.0 : (price_change < 0) ? -1.0 : 0.0;
        double trix_direction = (trix_change > 0) ? 1.0 : (trix_change < 0) ? -1.0 : 0.0;
        
        // Divergence occurs when directions are opposite
        if (price_direction != trix_direction && price_direction != 0.0 && trix_direction != 0.0) {
            return trix_direction - price_direction;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double TRIX::getTrendDirection() const {
    double trix_value = get(0);
    if (isNaN(trix_value)) {
        return 0.0;
    }
    
    if (trix_value > 0.0) {
        return 1.0;   // Uptrend
    } else if (trix_value < 0.0) {
        return -1.0;  // Downtrend
    } else {
        return 0.0;   // Neutral
    }
}

double TRIX::calculateEMA(double current_value, double prev_ema, bool& has_prev) {
    if (!has_prev) {
        has_prev = true;
        return current_value;
    } else {
        return current_value * multiplier_ + prev_ema * (1.0 - multiplier_);
    }
}

} // namespace backtrader