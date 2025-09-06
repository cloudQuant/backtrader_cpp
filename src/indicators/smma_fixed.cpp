#include "indicators/smma.h"
#include <numeric>
#include <limits>

namespace backtrader {
namespace indicators {

// SmoothedMovingAverage implementation
SmoothedMovingAverage::SmoothedMovingAverage() 
    : Indicator(), prev_smma_(0.0), initialized_(false), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 1.0 / params.period;
    alpha1_ = 1.0 - alpha_;
    
    _minperiod(params.period);
}

SmoothedMovingAverage::SmoothedMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), prev_smma_(0.0), initialized_(false), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 1.0 / params.period;
    alpha1_ = 1.0 - alpha_;
    
    _minperiod(params.period);
}

SmoothedMovingAverage::SmoothedMovingAverage(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), prev_smma_(0.0), initialized_(false), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 1.0 / params.period;
    alpha1_ = 1.0 - alpha_;
    
    _minperiod(params.period);
    
    // Connect to data source for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

double SmoothedMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto smma_line = lines->getline(smma);
    if (!smma_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*smma_line)[ago];
}

double SmoothedMovingAverage::getSMMA(int ago) const {
    return get(ago);
}

double SmoothedMovingAverage::operator[](int ago) const {
    return get(ago);
}

size_t SmoothedMovingAverage::minbuffer() const {
    return params.period;
}

int SmoothedMovingAverage::getminperiod() const {
    return params.period;
}

size_t SmoothedMovingAverage::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto smma_line = lines->getline(smma);
    if (!smma_line) {
        return 0;
    }
    return smma_line->size();
}

void SmoothedMovingAverage::calculate() {
    if (data_source_) {
        // For LineSeries constructor, calculate for entire dataset
        if (data_source_->lines && data_source_->lines->size() > 0) {
            auto data_line = data_source_->lines->getline(0);
            
            // Set up datas for once() method
            if (datas.empty()) {
                datas.push_back(data_source_);
            }
            
            // Calculate SMMA for the entire dataset using once() method
            once(0, data_line->size());
        }
    } else if (data && data->lines && data->lines->size() > 0) {
        // For test framework constructor, calculate for entire dataset
        auto data_line = data->lines->getline(data->lines->size() > 4 ? 4 : 0);
        
        // Calculate SMMA for the entire dataset using once() method
        once(0, data_line->size());
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void SmoothedMovingAverage::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void SmoothedMovingAverage::prenext() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line) return;
    
    // Collect seed values for initial calculation
    seed_values_.push_back((*data_line)[0]);
    
    // Keep only the period we need
    if (seed_values_.size() > params.period) {
        seed_values_.erase(seed_values_.begin());
    }
    
    Indicator::prenext();
}

void SmoothedMovingAverage::nextstart() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto smma_line = lines->getline(smma);
    if (!smma_line) return;
    
    // Calculate initial SMMA as simple average of first period values
    if (seed_values_.size() >= params.period) {
        double sum = std::accumulate(seed_values_.begin(), seed_values_.end(), 0.0);
        prev_smma_ = sum / params.period;
        smma_line->set(0, prev_smma_);
        initialized_ = true;
    }
}

void SmoothedMovingAverage::next() {
    if (!initialized_) {
        nextstart();
        return;
    }
    
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto smma_line = lines->getline(smma);
    
    if (!data_line || !smma_line) return;
    
    double current_data = (*data_line)[0];
    
    // SMMA formula: new_value = (old_value * (period - 1) + new_data) / period
    // Which is equivalent to: new_value = old_value * alpha1 + new_data * alpha
    double smma_value = prev_smma_ * alpha1_ + current_data * alpha_;
    
    smma_line->set(0, smma_value);
    prev_smma_ = smma_value;
}

void SmoothedMovingAverage::once(int start, int end) {
    std::shared_ptr<LineSingle> data_line;
    
    // First try to get data line from data_source_ (for LineSeries input)
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        data_line = data_source_->lines->getline(0);
    }
    // Otherwise try from data (for DataSeries input)
    else if (data && data->lines && data->lines->size() > 0) {
        // Use the close price line (index 3) for OHLC data, or primary line (index 0) for simple data
        data_line = data->lines->getline(data->lines->size() > 4 ? 4 : 0);
    }
    
    auto smma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(smma));
    if (!smma_line || !data_line) {
        return;
    }
    
    // Clear the SMMA line buffer and reset to initial state
    smma_line->reset();
    
    // Process data in forward order
    for (int i = start; i < end; ++i) {
        double smma_value = std::numeric_limits<double>::quiet_NaN();
        
        // Calculate SMMA only when we have enough data
        if (i >= params.period - 1) {
            if (i == params.period - 1) {
                // Initial SMMA: simple average of first period values
                double sum = 0.0;
                int valid_count = 0;
                
                for (int j = 0; j < params.period; ++j) {
                    int data_index = i - params.period + 1 + j;
                    
                    if (data_index >= 0 && data_index < static_cast<int>(data_line->size())) {
                        double value = (*data_line)[data_index];
                        if (!std::isnan(value)) {
                            sum += value;
                            valid_count++;
                        }
                    }
                }
                
                if (valid_count == params.period) {
                    smma_value = sum / params.period;
                }
            } else {
                // Subsequent SMMA: exponential smoothing
                // Get previous SMMA value (last appended value)
                int prev_idx = smma_line->size() - 1;
                if (prev_idx >= 0) {
                    double prev_smma = (*smma_line)[prev_idx];
                    if (!std::isnan(prev_smma)) {
                        // Get current data value
                        double current_data = (*data_line)[i];
                        if (!std::isnan(current_data)) {
                            // SMMA formula: new_value = (old_value * (period - 1) + new_data) / period
                            smma_value = prev_smma * alpha1_ + current_data * alpha_;
                        }
                    }
                }
            }
        }
        
        // Use append to add value to buffer
        smma_line->append(smma_value);
    }
}

} // namespace indicators
} // namespace backtrader