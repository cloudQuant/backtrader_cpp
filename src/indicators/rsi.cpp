#include "indicators/rsi.h"
#include <algorithm>
#include <limits>
#include <numeric>

namespace backtrader {
namespace indicators {

RSI::RSI(int period) : Indicator(), period(period), avg_gain_(0.0), avg_loss_(0.0), 
                       prev_value_(std::numeric_limits<double>::quiet_NaN()), 
                       first_calculation_(true), data_source_(nullptr), current_index_(0) {
    // Set minimum period (need period + 1 for first RSI calculation)
    _minperiod(period + 1);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("rsi", 0);
    }
}

RSI::RSI(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), period(period), avg_gain_(0.0), avg_loss_(0.0), 
      prev_value_(std::numeric_limits<double>::quiet_NaN()), 
      first_calculation_(true), data_source_(data_source), current_index_(0) {
    // Set minimum period (need period + 1 for first RSI calculation)
    _minperiod(period + 1);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("rsi", 0);
    }
    
    // Set data member for compatibility with once() method
    data = data_source_;
}

RSI::RSI(std::shared_ptr<LineRoot> data) 
    : Indicator(), period(14), avg_gain_(0.0), avg_loss_(0.0), 
      prev_value_(std::numeric_limits<double>::quiet_NaN()), 
      first_calculation_(true), data_source_(nullptr), current_index_(0) {
    // Set minimum period (need period + 1 for first RSI calculation)
    _minperiod(period + 1);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("rsi", 0);
    }
}

RSI::RSI(std::shared_ptr<LineRoot> data, int period) 
    : Indicator(), period(period), avg_gain_(0.0), avg_loss_(0.0), 
      prev_value_(std::numeric_limits<double>::quiet_NaN()), 
      first_calculation_(true), data_source_(nullptr), current_index_(0) {
    // Set minimum period (need period + 1 for first RSI calculation)
    _minperiod(period + 1);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("rsi", 0);
    }
}

void RSI::next() {
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    double current_value = (*data->lines->getline(0))[0];
    
    if (std::isnan(current_value)) {
        return;
    }
    
    if (!std::isnan(prev_value_)) {
        double change = current_value - prev_value_;
        double gain = std::max(0.0, change);
        double loss = std::max(0.0, -change);
        
        gains_.push_back(gain);
        losses_.push_back(loss);
        
        if (static_cast<int>(gains_.size()) > period) {
            gains_.pop_front();
            losses_.pop_front();
        }
        
        if (static_cast<int>(gains_.size()) == period) {
            if (first_calculation_) {
                // Initial average calculation
                avg_gain_ = std::accumulate(gains_.begin(), gains_.end(), 0.0) / period;
                avg_loss_ = std::accumulate(losses_.begin(), losses_.end(), 0.0) / period;
                first_calculation_ = false;
            } else {
                // Wilder's smoothing
                avg_gain_ = (avg_gain_ * (period - 1) + gain) / period;
                avg_loss_ = (avg_loss_ * (period - 1) + loss) / period;
            }
            
            double rsi_value = 0.0;
            if (avg_loss_ != 0.0) {
                double rs = avg_gain_ / avg_loss_;
                rsi_value = 100.0 - (100.0 / (1.0 + rs));
            } else {
                rsi_value = 100.0;
            }
            
            lines->getline(0)->set(0, rsi_value);
        }
    }
    
    prev_value_ = current_value;
}

void RSI::once(int start, int end) {
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    auto data_line = data->lines->getline(0);
    auto rsi_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!rsi_line) {
        return;
    }
    
    std::deque<double> gains, losses;
    double avg_gain = 0.0, avg_loss = 0.0;
    double prev_val = std::numeric_limits<double>::quiet_NaN();
    bool first_calc = true;
    bool output_started = false;
    
    for (int i = start; i < end; ++i) {
        double value = (*data_line)[i];
        
        if (!std::isnan(value) && !std::isnan(prev_val)) {
            double change = value - prev_val;
            double gain = std::max(0.0, change);
            double loss = std::max(0.0, -change);
            
            gains.push_back(gain);
            losses.push_back(loss);
            
            if (static_cast<int>(gains.size()) > period) {
                gains.pop_front();
                losses.pop_front();
            }
            
            if (static_cast<int>(gains.size()) == period) {
                if (first_calc) {
                    avg_gain = std::accumulate(gains.begin(), gains.end(), 0.0) / period;
                    avg_loss = std::accumulate(losses.begin(), losses.end(), 0.0) / period;
                    first_calc = false;
                } else {
                    avg_gain = (avg_gain * (period - 1) + gain) / period;
                    avg_loss = (avg_loss * (period - 1) + loss) / period;
                }
                
                double rsi_value = 0.0;
                if (avg_loss != 0.0) {
                    double rs = avg_gain / avg_loss;
                    rsi_value = 100.0 - (100.0 / (1.0 + rs));
                } else {
                    rsi_value = 100.0;
                }
                
                if (!output_started) {
                    // Overwrite the initial NaN value for the first calculated RSI
                    rsi_line->set(0, rsi_value);
                    output_started = true;
                } else {
                    rsi_line->append(rsi_value);
                }
            }
        }
        
        if (!std::isnan(value)) {
            prev_val = value;
        }
    }
}

std::vector<std::string> RSI::_get_line_names() const {
    return {"rsi"};
}

double RSI::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto rsi_line = lines->getline(0);
    if (!rsi_line || rsi_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Convert ago to positive index from current position
    // ago=0 means last value, ago=-1 means one before last, etc.
    int index;
    if (ago <= 0) {
        index = static_cast<int>(rsi_line->size()) - 1 + ago;
    } else {
        // Positive ago means going forward from current position (unusual)
        index = static_cast<int>(rsi_line->size()) - 1 + ago;
    }
    
    if (index < 0 || index >= static_cast<int>(rsi_line->size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*rsi_line)[index];
}

void RSI::calculate() {
    if (data_source_) {
        // For data source constructor, calculate for entire dataset
        if (data_source_->lines && data_source_->lines->size() > 0) {
            auto data_line = data_source_->lines->getline(0);
            
            // Calculate RSI for the entire dataset using once() method
            once(0, data_line->size());
        }
    } else {
        // For normal constructor, use the existing next() logic
        next();
    }
}

double RSI::getOverboughtOversoldStatus() const {
    double current_rsi = get(0);
    
    if (std::isnan(current_rsi)) {
        return 0.0;  // Neutral if no valid RSI value
    }
    
    if (current_rsi > 70.0) {
        return 1.0;  // Overbought
    } else if (current_rsi < 30.0) {
        return -1.0; // Oversold
    } else {
        return 0.0;  // Neutral
    }
}

} // namespace indicators
} // namespace backtrader