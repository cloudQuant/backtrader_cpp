#include "indicators/wmaenvelope.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

WMAEnvelope::WMAEnvelope() : current_index_(0) {
    // Default parameters
    params.period = 30;
    params.perc = 2.5;
}

WMAEnvelope::WMAEnvelope(std::shared_ptr<LineIterator> data_line, 
                         int period, double perc) 
    : current_index_(0) {
    params.period = period;
    params.perc = perc;
    
    // Create WMA indicator - cast to LineSeries for constructor compatibility
    auto data_series = std::dynamic_pointer_cast<LineSeries>(data_line);
    if (data_series) {
        wma_ = std::make_shared<WMA>(data_series, period);
    } else {
        // Fallback to LineRoot if cast fails
        auto data_root = std::dynamic_pointer_cast<LineRoot>(data_line);
        if (data_root) {
            wma_ = std::make_shared<WMA>(data_root, period);
        }
    }
}

WMAEnvelope::WMAEnvelope(std::shared_ptr<LineRoot> data_line, 
                         int period, double perc) 
    : current_index_(0) {
    params.period = period;
    params.perc = perc;
    
    // Create WMA indicator directly with LineRoot
    if (data_line) {
        wma_ = std::make_shared<WMA>(data_line, period);
    }
}

double WMAEnvelope::get(int ago) const {
    return getMid(ago);
}

double WMAEnvelope::getMid(int ago) const {
    // Return NaN for now - this is a placeholder implementation
    return std::numeric_limits<double>::quiet_NaN();
}

double WMAEnvelope::getUpper(int ago) const {
    // Return NaN for now - this is a placeholder implementation
    return std::numeric_limits<double>::quiet_NaN();
}

double WMAEnvelope::getLower(int ago) const {
    // Return NaN for now - this is a placeholder implementation
    return std::numeric_limits<double>::quiet_NaN();
}

void WMAEnvelope::calculate() {
    // Calculate WMA
    if (wma_) {
        wma_->calculate();
    }
    
    // Get current WMA value
    double wma_val = wma_ ? wma_->get(0) : std::numeric_limits<double>::quiet_NaN();
    
    // Calculate envelope values
    double upper_val, lower_val;
    if (std::isnan(wma_val)) {
        upper_val = std::numeric_limits<double>::quiet_NaN();
        lower_val = std::numeric_limits<double>::quiet_NaN();
    } else {
        double envelope_offset = wma_val * (params.perc / 100.0);
        upper_val = wma_val + envelope_offset;
        lower_val = wma_val - envelope_offset;
    }
    
    // Store values (minimal implementation to fix compilation)
    upper_data_.push_back(upper_val);
    lower_data_.push_back(lower_val);
    current_index_ = static_cast<int>(upper_data_.size()) - 1;
}

} // namespace indicators
} // namespace backtrader