#include "indicators/MFI.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace backtrader {

MFI::MFI(std::shared_ptr<LineRoot> high_input,
         std::shared_ptr<LineRoot> low_input,
         std::shared_ptr<LineRoot> close_input,
         std::shared_ptr<LineRoot> volume_input,
         size_t period)
    : IndicatorBase(high_input, "MFI"),
      period_(period),
      high_buffer_(period + 1),
      low_buffer_(period + 1),
      close_buffer_(period + 1),
      volume_buffer_(period + 1),
      tp_buffer_(period + 1),
      raw_mf_buffer_(period),
      mf_direction_buffer_(period),
      positive_mf_sum_(0.0),
      negative_mf_sum_(0.0) {
    
    if (period == 0) {
        throw std::invalid_argument("MFI period must be greater than 0");
    }
    
    setInputs(high_input, low_input, close_input, volume_input);
    setParam("period", static_cast<double>(period));
    setMinPeriod(period + 1);
}

void MFI::setInputs(std::shared_ptr<LineRoot> high_input,
                    std::shared_ptr<LineRoot> low_input,
                    std::shared_ptr<LineRoot> close_input,
                    std::shared_ptr<LineRoot> volume_input) {
    if (!high_input || !low_input || !close_input || !volume_input) {
        throw std::invalid_argument("MFI requires valid high, low, close, and volume inputs");
    }
    
    // Store additional inputs (base class stores first input)
    addInput(low_input);
    addInput(close_input);
    addInput(volume_input);
}

void MFI::reset() {
    IndicatorBase::reset();
    high_buffer_.clear();
    low_buffer_.clear();
    close_buffer_.clear();
    volume_buffer_.clear();
    tp_buffer_.clear();
    raw_mf_buffer_.clear();
    mf_direction_buffer_.clear();
    
    positive_mf_sum_ = 0.0;
    negative_mf_sum_ = 0.0;
}

void MFI::calculate() {
    if (inputs_.size() < 4) {
        setOutput(0, NaN);
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    auto close_input = getInput(2);
    auto volume_input = getInput(3);
    
    if (!high_input || !low_input || !close_input || !volume_input) {
        setOutput(0, NaN);
        return;
    }
    
    double current_high = high_input->get(0);
    double current_low = low_input->get(0);
    double current_close = close_input->get(0);
    double current_volume = volume_input->get(0);
    
    if (isNaN(current_high) || isNaN(current_low) || isNaN(current_close) || isNaN(current_volume)) {
        setOutput(0, NaN);
        return;
    }
    
    // Add current values to buffers
    high_buffer_.push_back(current_high);
    low_buffer_.push_back(current_low);
    close_buffer_.push_back(current_close);
    volume_buffer_.push_back(current_volume);
    
    // Calculate typical price
    double typical_price = calculateTypicalPrice(current_high, current_low, current_close);
    tp_buffer_.push_back(typical_price);
    
    if (tp_buffer_.size() < 2) {
        setOutput(0, NaN);
        return;
    }
    
    // Determine money flow direction
    double prev_tp = tp_buffer_[tp_buffer_.size() - 2];
    int direction;
    if (typical_price > prev_tp) {
        direction = 1;  // Positive money flow
    } else if (typical_price < prev_tp) {
        direction = -1; // Negative money flow
    } else {
        direction = 0;  // No change
    }
    
    // Calculate raw money flow
    double raw_mf = typical_price * current_volume;
    
    raw_mf_buffer_.push_back(raw_mf);
    mf_direction_buffer_.push_back(direction);
    
    // Update money flow sums
    updateMoneyFlowSums(raw_mf, direction);
    
    if (raw_mf_buffer_.size() < period_) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate Money Flow Index
    if (negative_mf_sum_ == 0.0) {
        setOutput(0, 100.0);  // All positive money flow
    } else {
        double money_flow_ratio = positive_mf_sum_ / negative_mf_sum_;
        double mfi = 100.0 - (100.0 / (1.0 + money_flow_ratio));
        setOutput(0, mfi);
    }
}

void MFI::calculateBatch(size_t start, size_t end) {
    if (inputs_.size() < 4) {
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    auto close_input = getInput(2);
    auto volume_input = getInput(3);
    
    if (!high_input || !low_input || !close_input || !volume_input) {
        return;
    }
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            high_input->forward();
            low_input->forward();
            close_input->forward();
            volume_input->forward();
        }
    }
}

void MFI::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("MFI period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(period + 1);
    
    // Resize buffers and reset
    high_buffer_ = CircularBuffer<double>(period + 1);
    low_buffer_ = CircularBuffer<double>(period + 1);
    close_buffer_ = CircularBuffer<double>(period + 1);
    volume_buffer_ = CircularBuffer<double>(period + 1);
    tp_buffer_ = CircularBuffer<double>(period + 1);
    raw_mf_buffer_ = CircularBuffer<double>(period);
    mf_direction_buffer_ = CircularBuffer<int>(period);
    reset();
}

double MFI::getOverboughtOversold(double overbought_level, double oversold_level) const {
    double mfi_value = get(0);
    if (isNaN(mfi_value)) {
        return 0.0;
    }
    
    if (mfi_value >= overbought_level) {
        return 1.0;   // Overbought
    } else if (mfi_value <= oversold_level) {
        return -1.0;  // Oversold
    } else {
        return 0.0;   // Neutral
    }
}

double MFI::getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback) const {
    if (!price_line || lookback < 3) {
        return 0.0;
    }
    
    try {
        // Find recent extremes in both price and MFI
        double price_high = price_line->get(0);
        double price_low = price_line->get(0);
        double mfi_high = get(0);
        double mfi_low = get(0);
        
        for (size_t i = 1; i < lookback; ++i) {
            double price = price_line->get(-static_cast<int>(i));
            double mfi = get(-static_cast<int>(i));
            
            if (!isNaN(price)) {
                price_high = std::max(price_high, price);
                price_low = std::min(price_low, price);
            }
            if (!isNaN(mfi)) {
                mfi_high = std::max(mfi_high, mfi);
                mfi_low = std::min(mfi_low, mfi);
            }
        }
        
        double price_range = price_high - price_low;
        double mfi_range = mfi_high - mfi_low;
        
        if (price_range == 0.0 || mfi_range == 0.0) {
            return 0.0;
        }
        
        // Calculate trend directions
        double price_trend = (price_line->get(0) - price_line->get(-static_cast<int>(lookback-1))) / price_range;
        double mfi_trend = (get(0) - get(-static_cast<int>(lookback-1))) / mfi_range;
        
        // Divergence strength
        return mfi_trend - price_trend;
        
    } catch (...) {
        return 0.0;
    }
}

double MFI::getMoneyFlowRatio() const {
    if (negative_mf_sum_ == 0.0) {
        return std::numeric_limits<double>::infinity();
    }
    return positive_mf_sum_ / negative_mf_sum_;
}

double MFI::getRawMoneyFlow() const {
    if (raw_mf_buffer_.empty()) {
        return NaN;
    }
    return raw_mf_buffer_.back();
}

double MFI::getMomentum(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 3;
        
        double mfi_current = get(0);
        double mfi_past = get(-static_cast<int>(lookback));
        
        if (isNaN(mfi_current) || isNaN(mfi_past)) {
            return 0.0;
        }
        
        return mfi_current - mfi_past;
        
    } catch (...) {
        return 0.0;
    }
}

bool MFI::isExtreme(double extreme_high, double extreme_low) const {
    double mfi_value = get(0);
    if (isNaN(mfi_value)) {
        return false;
    }
    
    return mfi_value >= extreme_high || mfi_value <= extreme_low;
}

double MFI::getTrendStrength() const {
    double mfi_value = get(0);
    if (isNaN(mfi_value)) {
        return 0.0;
    }
    
    // Distance from neutral (50) indicates trend strength
    double distance_from_neutral = std::abs(mfi_value - 50.0);
    return (distance_from_neutral / 50.0) * 100.0;
}

double MFI::calculateTypicalPrice(double high, double low, double close) const {
    return (high + low + close) / 3.0;
}

void MFI::updateMoneyFlowSums(double raw_mf, int direction) {
    // Add current money flow
    if (direction > 0) {
        positive_mf_sum_ += raw_mf;
    } else if (direction < 0) {
        negative_mf_sum_ += raw_mf;
    }
    // If direction == 0, no change to either sum
    
    // Remove oldest money flow if buffer is full
    if (raw_mf_buffer_.size() > period_) {
        double old_raw_mf = raw_mf_buffer_[raw_mf_buffer_.size() - period_ - 1];
        int old_direction = mf_direction_buffer_[mf_direction_buffer_.size() - period_ - 1];
        
        if (old_direction > 0) {
            positive_mf_sum_ -= old_raw_mf;
        } else if (old_direction < 0) {
            negative_mf_sum_ -= old_raw_mf;
        }
        
        // Ensure sums don't go negative due to floating point errors
        positive_mf_sum_ = std::max(0.0, positive_mf_sum_);
        negative_mf_sum_ = std::max(0.0, negative_mf_sum_);
    }
}

} // namespace backtrader