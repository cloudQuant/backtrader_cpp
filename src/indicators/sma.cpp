#include "indicators/sma.h"
#include <numeric>

namespace backtrader {
namespace indicators {

SMA::SMA(int period) : Indicator(), period(period), sum_(0.0), current_index_(0) {
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("sma", 0);
    }
}

SMA::SMA(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), period(period), sum_(0.0), data_source_(data_source), current_index_(0) {
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("sma", 0);
    }
    
    // Set data member for compatibility with once() method
    data = data_source_;
    
    // SMA line buffer will be populated during calculation
    // Do not pre-extend as it would fill with NaN values
}

void SMA::next() {
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    double current_value = (*data->lines->getline(0))[0];
    
    if (std::isnan(current_value)) {
        return;
    }
    
    values_.push_back(current_value);
    sum_ += current_value;
    
    if (static_cast<int>(values_.size()) > period) {
        sum_ -= values_.front();
        values_.pop_front();
    }
    
    if (static_cast<int>(values_.size()) == period) {
        double sma_value = sum_ / period;
        lines->getline(0)->set(0, sma_value);
    }
}

void SMA::once(int start, int end) {
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    auto data_line = data->lines->getline(0);
    auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!sma_line) {
        return;
    }
    
    std::deque<double> window;
    double window_sum = 0.0;
    bool first_value = true;
    
    for (int i = start; i < end; ++i) {
        double value = (*data_line)[i];
        
        if (!std::isnan(value)) {
            window.push_back(value);
            window_sum += value;
            
            if (static_cast<int>(window.size()) > period) {
                window_sum -= window.front();
                window.pop_front();
            }
            
            // Only append values once we have enough data for SMA calculation
            if (static_cast<int>(window.size()) == period) {
                double sma_value = window_sum / period;
                if (first_value) {
                    // Overwrite the initial NaN value for the first calculated SMA
                    sma_line->set(0, sma_value);
                    first_value = false;
                } else {
                    sma_line->append(sma_value);
                }
            }
        }
    }
}

std::vector<std::string> SMA::_get_line_names() const {
    return {"sma"};
}

double SMA::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto sma_line = lines->getline(0);
    if (!sma_line || sma_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Convert ago to positive index from current position
    // ago=0 means last value, ago=-1 means one before last, etc.
    int index;
    if (ago <= 0) {
        index = static_cast<int>(sma_line->size()) - 1 + ago;
    } else {
        // Positive ago means going forward from current position (unusual)
        index = static_cast<int>(sma_line->size()) - 1 + ago;
    }
    
    if (index < 0 || index >= static_cast<int>(sma_line->size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*sma_line)[index];
}

void SMA::calculate() {
    if (data_source_) {
        // For data source constructor, calculate for entire dataset
        if (data_source_->lines && data_source_->lines->size() > 0) {
            auto data_line = data_source_->lines->getline(0);
            
            // Calculate SMA for the entire dataset using once() method
            once(0, data_line->size());
        }
    } else {
        // For normal constructor, use the existing next() logic
        next();
    }
}

} // namespace indicators
} // namespace backtrader