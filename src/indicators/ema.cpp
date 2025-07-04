#include "indicators/ema.h"
#include <limits>

namespace backtrader {
namespace indicators {

EMA::EMA(int period) : Indicator(), period(period), first_value_(true), ema_value_(0.0), data_source_(nullptr), current_index_(0) {
    // Calculate alpha (smoothing factor) - same as Python implementation
    alpha = 2.0 / (1.0 + period);
    
    // Set minimum period to the period parameter 
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("ema", 0);
    }
}

EMA::EMA(std::shared_ptr<LineSeries> data_source, int period) : Indicator(), period(period), first_value_(true), ema_value_(0.0), data_source_(data_source), current_index_(0) {
    // Calculate alpha (smoothing factor) - same as Python implementation
    alpha = 2.0 / (1.0 + period);
    
    // Set minimum period to the period parameter 
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("ema", 0);
    }
    
    // Set data member for compatibility with once() method
    data = data_source_;
}

void EMA::next() {
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    double current_value = (*data->lines->getline(0))[0];
    
    if (std::isnan(current_value)) {
        return;
    }
    
    if (first_value_) {
        ema_value_ = current_value;
        first_value_ = false;
    } else {
        ema_value_ = alpha * current_value + (1.0 - alpha) * ema_value_;
    }
    
    lines->getline(0)->set(0, ema_value_);
}

void EMA::once(int start, int end) {
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    auto data_line = data->lines->getline(0);
    auto ema_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!ema_line) {
        return;
    }
    
    std::deque<double> window;
    double window_sum = 0.0;
    double ema_value = 0.0;
    bool first_value = true;
    bool ema_started = false;
    
    for (int i = start; i < end; ++i) {
        double value = (*data_line)[i];
        
        if (!std::isnan(value)) {
            if (!ema_started) {
                // Accumulate values for initial SMA calculation
                window.push_back(value);
                window_sum += value;
                
                if (static_cast<int>(window.size()) > period) {
                    window_sum -= window.front();
                    window.pop_front();
                }
                
                // Once we have enough data for initial EMA calculation
                if (static_cast<int>(window.size()) == period) {
                    // Use SMA of first period values as initial EMA
                    ema_value = window_sum / period;
                    
                    if (first_value) {
                        ema_line->set(0, ema_value);
                        first_value = false;
                    } else {
                        ema_line->append(ema_value);
                    }
                    ema_started = true;
                }
            } else {
                // Use EMA formula for subsequent values
                ema_value = alpha * value + (1.0 - alpha) * ema_value;
                ema_line->append(ema_value);
            }
        }
    }
}

std::vector<std::string> EMA::_get_line_names() const {
    return {"ema"};
}

double EMA::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto ema_line = lines->getline(0);
    if (!ema_line || ema_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Convert ago to positive index from current position
    // ago=0 means last value, ago=-1 means one before last, etc.
    int index;
    if (ago <= 0) {
        index = static_cast<int>(ema_line->size()) - 1 + ago;
    } else {
        // Positive ago means going forward from current position (unusual)
        index = static_cast<int>(ema_line->size()) - 1 + ago;
    }
    
    if (index < 0 || index >= static_cast<int>(ema_line->size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*ema_line)[index];
}

void EMA::calculate() {
    if (data_source_) {
        // For data source constructor, calculate for entire dataset
        if (data_source_->lines && data_source_->lines->size() > 0) {
            auto data_line = data_source_->lines->getline(0);
            
            // Calculate EMA for the entire dataset using once() method
            once(0, data_line->size());
        }
    } else {
        // For normal constructor, use the existing next() logic
        next();
    }
}

} // namespace indicators
} // namespace backtrader