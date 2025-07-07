#include "indicators/wmaosc.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

WMAOsc::WMAOsc() {
    // Default parameters
    params.period1 = 14;
    params.period2 = 28;
}

WMAOsc::WMAOsc(std::shared_ptr<LineIterator> data_line, 
               int period1, int period2) {
    params.period1 = period1;
    params.period2 = period2;
    
    // Create WMA indicators - cast to LineSeries for constructor compatibility
    auto data_series = std::dynamic_pointer_cast<LineSeries>(data_line);
    if (data_series) {
        wma1_ = std::make_shared<WMA>(data_series, period1);
        wma2_ = std::make_shared<WMA>(data_series, period2);
    } else {
        // Fallback to LineRoot if cast fails
        auto data_root = std::dynamic_pointer_cast<LineRoot>(data_line);
        if (data_root) {
            wma1_ = std::make_shared<WMA>(data_root, period1);
            wma2_ = std::make_shared<WMA>(data_root, period2);
        }
    }
}

WMAOsc::WMAOsc(std::shared_ptr<LineRoot> data_line, 
               int period1, int period2) {
    params.period1 = period1;
    params.period2 = period2;
    
    // Create WMA indicators directly with LineRoot
    if (data_line) {
        wma1_ = std::make_shared<WMA>(data_line, period1);
        wma2_ = std::make_shared<WMA>(data_line, period2);
    }
}

double WMAOsc::get(int ago) const {
    // Return NaN for now - this is a placeholder implementation
    return std::numeric_limits<double>::quiet_NaN();
}

void WMAOsc::calculate() {
    // Calculate both WMAs
    if (wma1_) {
        wma1_->calculate();
    }
    if (wma2_) {
        wma2_->calculate();
    }
    
    // Get current values
    double wma1_val = wma1_ ? wma1_->get(0) : std::numeric_limits<double>::quiet_NaN();
    double wma2_val = wma2_ ? wma2_->get(0) : std::numeric_limits<double>::quiet_NaN();
    
    // Calculate oscillator value
    double osc_value;
    if (std::isnan(wma1_val) || std::isnan(wma2_val)) {
        osc_value = std::numeric_limits<double>::quiet_NaN();
    } else {
        osc_value = wma1_val - wma2_val;
    }
    
    // For now, we'll just store the result (proper implementation would use lines)
    // This is a minimal implementation to fix compilation
}

} // namespace indicators
} // namespace backtrader