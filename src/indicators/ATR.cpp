#include "indicators/ATR.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace backtrader {

ATR::ATR(std::shared_ptr<LineRoot> high_input,
         std::shared_ptr<LineRoot> low_input,
         std::shared_ptr<LineRoot> close_input,
         size_t period,
         bool use_ema)
    : IndicatorBase(close_input, "ATR"),
      period_(period),
      use_ema_(use_ema),
      high_line_(high_input),
      low_line_(low_input),
      close_line_(close_input),
      tr_sum_(0.0),
      prev_close_(NaN),
      has_prev_close_(false) {
    
    if (period == 0) {
        throw std::invalid_argument("ATR period must be greater than 0");
    }
    
    if (!high_input || !low_input || !close_input) {
        throw std::invalid_argument("High, low, and close price lines are required for ATR");
    }
    
    // Set parameters
    setParam("period", static_cast<double>(period));
    setParam("use_ema", use_ema ? 1.0 : 0.0);
    
    // ATR needs at least period+1 data points
    setMinPeriod(period + 1);
    
    // Initialize EMA indicator if using EMA method
    if (use_ema_) {
        tr_line_ = std::make_shared<LineRoot>(1000, "true_range");
        tr_ema_ = std::make_unique<EMA>(tr_line_, period);
    }
}

void ATR::reset() {
    IndicatorBase::reset();
    
    tr_sum_ = 0.0;
    prev_close_ = NaN;
    has_prev_close_ = false;
    tr_buffer_.clear();
    
    if (use_ema_) {
        if (tr_line_) tr_line_->home();
        if (tr_ema_) tr_ema_->reset();
    }
}

void ATR::calculate() {
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
    
    // First data point, cannot calculate ATR
    if (!has_prev_close_) {
        prev_close_ = current_close;
        has_prev_close_ = true;
        setOutput(0, NaN);
        return;
    }
    
    // Calculate true range
    double true_range = getCurrentTR();
    if (isNaN(true_range)) {
        setOutput(0, NaN);
        return;
    }
    
    if (use_ema_) {
        calculateWithEMA(true_range);
    } else {
        calculateWithSMA(true_range);
    }
    
    prev_close_ = current_close;
}

void ATR::calculateBatch(size_t start, size_t end) {
    if (!hasValidInput() || !high_line_ || !low_line_) {
        return;
    }
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            // Advance all input data lines
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
}

void ATR::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("ATR period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period + 1);
    
    // Reset state
    reset();
}

double ATR::getCurrentTR() const {
    if (!has_prev_close_ || !high_line_ || !low_line_ || !close_line_) {
        return NaN;
    }
    
    double current_high = high_line_->get(0);
    double current_low = low_line_->get(0);
    
    if (isNaN(current_high) || isNaN(current_low) || isNaN(prev_close_)) {
        return NaN;
    }
    
    // Calculate the three candidates for true range
    double hl = current_high - current_low;
    double hc = std::abs(current_high - prev_close_);
    double lc = std::abs(current_low - prev_close_);
    
    return std::max({hl, hc, lc});
}

double ATR::getRelativeATR(double reference_price) const {
    double atr_value = get(0);
    if (isNaN(atr_value)) {
        return NaN;
    }
    
    if (reference_price == 0.0) {
        reference_price = close_line_->get(0);
    }
    
    if (isNaN(reference_price) || reference_price == 0.0) {
        return NaN;
    }
    
    return (atr_value / reference_price) * 100.0;
}

std::pair<double, double> ATR::getATRChannel(double multiplier, double price_base) const {
    double atr_value = get(0);
    if (isNaN(atr_value)) {
        return {NaN, NaN};
    }
    
    if (price_base == 0.0) {
        price_base = close_line_->get(0);
    }
    
    if (isNaN(price_base)) {
        return {NaN, NaN};
    }
    
    double atr_distance = atr_value * multiplier;
    return {price_base + atr_distance, price_base - atr_distance};
}

double ATR::getTrendStrength(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 1;
        
        double current_atr = get(0);
        double past_atr = get(-static_cast<int>(lookback));
        
        if (isNaN(current_atr) || isNaN(past_atr) || past_atr == 0.0) {
            return 0.0;
        }
        
        // Return percentage change in ATR
        return (current_atr - past_atr) / past_atr;
        
    } catch (...) {
        return 0.0;
    }
}

int ATR::getVolatilityLevel() const {
    double relative_atr = getRelativeATR();
    if (isNaN(relative_atr)) {
        return 0;
    }
    
    // Classify based on relative ATR value
    if (relative_atr < 1.0) {
        return 1;  // Low volatility
    } else if (relative_atr < 3.0) {
        return 2;  // Medium volatility
    } else {
        return 3;  // High volatility
    }
}

void ATR::calculateWithEMA(double true_range) {
    // Update TR data line and calculate EMA
    tr_line_->forward(true_range);
    tr_ema_->calculate();
    
    double atr_value = tr_ema_->get(0);
    setOutput(0, atr_value);
}

void ATR::calculateWithSMA(double true_range) {
    tr_buffer_.push_back(true_range);
    tr_sum_ += true_range;
    
    if (tr_buffer_.size() > period_) {
        tr_sum_ -= tr_buffer_.front();
        tr_buffer_.pop_front();
    }
    
    if (tr_buffer_.size() < period_) {
        setOutput(0, NaN);
        return;
    }
    
    double atr_value = tr_sum_ / period_;
    setOutput(0, atr_value);
}

} // namespace backtrader