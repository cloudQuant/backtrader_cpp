#include "indicators/OBV.h"
#include "Common.h"
#include <stdexcept>
#include <cmath>

namespace backtrader {

OBV::OBV(std::shared_ptr<LineRoot> close_input,
         std::shared_ptr<LineRoot> volume_input)
    : IndicatorBase(close_input, "OBV"),
      current_obv_(0.0),
      prev_close_(NaN),
      has_prev_close_(false) {
    
    setInputs(close_input, volume_input);
    setMinPeriod(1);
}

void OBV::setInputs(std::shared_ptr<LineRoot> close_input,
                    std::shared_ptr<LineRoot> volume_input) {
    if (!close_input || !volume_input) {
        throw std::invalid_argument("OBV requires valid close and volume inputs");
    }
    
    // Store additional input (base class stores first input)
    addInput(volume_input);
}

void OBV::reset() {
    IndicatorBase::reset();
    current_obv_ = 0.0;
    prev_close_ = NaN;
    has_prev_close_ = false;
}

void OBV::calculate() {
    if (inputs_.size() < 2) {
        setOutput(0, NaN);
        return;
    }
    
    auto close_input = getInput(0);
    auto volume_input = getInput(1);
    
    if (!close_input || !volume_input) {
        setOutput(0, NaN);
        return;
    }
    
    double current_close = close_input->get(0);
    double current_volume = volume_input->get(0);
    
    if (isNaN(current_close) || isNaN(current_volume)) {
        setOutput(0, NaN);
        return;
    }
    
    if (!has_prev_close_) {
        // First calculation - initialize with current volume
        current_obv_ = current_volume;
        prev_close_ = current_close;
        has_prev_close_ = true;
    } else {
        // Calculate OBV based on price direction
        if (current_close > prev_close_) {
            // Price up - add volume
            current_obv_ += current_volume;
        } else if (current_close < prev_close_) {
            // Price down - subtract volume
            current_obv_ -= current_volume;
        }
        // If price unchanged, OBV remains the same
        
        prev_close_ = current_close;
    }
    
    setOutput(0, current_obv_);
}

void OBV::calculateBatch(size_t start, size_t end) {
    if (inputs_.size() < 2) {
        return;
    }
    
    auto close_input = getInput(0);
    auto volume_input = getInput(1);
    
    if (!close_input || !volume_input) {
        return;
    }
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            close_input->forward();
            volume_input->forward();
        }
    }
}

double OBV::getTrendDirection() const {
    try {
        double obv_current = get(0);
        double obv_prev = get(-10);  // Look back 10 periods
        
        if (isNaN(obv_current) || isNaN(obv_prev)) {
            return 0.0;
        }
        
        if (obv_current > obv_prev) {
            return 1.0;   // Uptrend
        } else if (obv_current < obv_prev) {
            return -1.0;  // Downtrend
        } else {
            return 0.0;   // Sideways
        }
        
    } catch (...) {
        return 0.0;
    }
}

double OBV::getMomentum(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 10;
        
        double obv_current = get(0);
        double obv_past = get(-static_cast<int>(lookback));
        
        if (isNaN(obv_current) || isNaN(obv_past)) {
            return 0.0;
        }
        
        return obv_current - obv_past;
        
    } catch (...) {
        return 0.0;
    }
}

double OBV::getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback) const {
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
        
        // Calculate OBV trend
        double obv_current = get(0);
        double obv_past = get(-static_cast<int>(lookback));
        
        if (isNaN(obv_current) || isNaN(obv_past) || obv_past == 0.0) {
            return 0.0;
        }
        
        double obv_change = (obv_current - obv_past) / std::abs(obv_past);
        
        // Normalize directions for comparison
        double price_direction = (price_change > 0) ? 1.0 : (price_change < 0) ? -1.0 : 0.0;
        double obv_direction = (obv_change > 0) ? 1.0 : (obv_change < 0) ? -1.0 : 0.0;
        
        // Divergence occurs when directions are opposite
        if (price_direction != obv_direction && price_direction != 0.0 && obv_direction != 0.0) {
            // Positive divergence: price down but OBV up (bullish)
            // Negative divergence: price up but OBV down (bearish)
            return obv_direction - price_direction;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double OBV::getSlope(size_t lookback) const {
    try {
        if (lookback == 0) lookback = 5;
        
        double obv_current = get(0);
        double obv_past = get(-static_cast<int>(lookback));
        
        if (isNaN(obv_current) || isNaN(obv_past)) {
            return 0.0;
        }
        
        // Calculate slope as change per period
        return (obv_current - obv_past) / lookback;
        
    } catch (...) {
        return 0.0;
    }
}

double OBV::getVolumePressure() const {
    try {
        // Volume pressure based on recent OBV momentum
        double short_momentum = getMomentum(5);
        double long_momentum = getMomentum(20);
        
        if (short_momentum == 0.0 && long_momentum == 0.0) {
            return 0.0;
        }
        
        // Compare short-term vs long-term momentum
        if (std::abs(long_momentum) > 0.0) {
            return short_momentum / std::abs(long_momentum);
        } else {
            return (short_momentum > 0) ? 1.0 : -1.0;
        }
        
    } catch (...) {
        return 0.0;
    }
}

bool OBV::isConfirmingTrend(std::shared_ptr<LineRoot> price_line, size_t lookback) const {
    if (!price_line || lookback < 3) {
        return false;
    }
    
    try {
        // Check if OBV and price are moving in the same direction
        double price_momentum = 0.0;
        double obv_momentum = getMomentum(lookback);
        
        if (price_line) {
            double price_current = price_line->get(0);
            double price_past = price_line->get(-static_cast<int>(lookback));
            
            if (!isNaN(price_current) && !isNaN(price_past) && price_past != 0.0) {
                price_momentum = price_current - price_past;
            }
        }
        
        // Both should have the same sign (both positive or both negative)
        if (price_momentum > 0 && obv_momentum > 0) {
            return true;  // Both trending up
        } else if (price_momentum < 0 && obv_momentum < 0) {
            return true;  // Both trending down
        }
        
        return false;
        
    } catch (...) {
        return false;
    }
}

} // namespace backtrader