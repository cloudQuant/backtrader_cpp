#include "indicators/smma.h"
#include <numeric>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

// SmoothedMovingAverage implementation
SmoothedMovingAverage::SmoothedMovingAverage() 
    : Indicator(), prev_smma_(0.0), initialized_(false), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Calculate smoothing factors with higher precision
    alpha_ = 1.0 / static_cast<double>(params.period);
    alpha1_ = 1.0 - alpha_;
    
    _minperiod(params.period);
}

SmoothedMovingAverage::SmoothedMovingAverage(std::shared_ptr<LineSeries> data_source)
    : SmoothedMovingAverage(data_source, 30) {  // Delegate to main constructor with default period
}

SmoothedMovingAverage::SmoothedMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), prev_smma_(0.0), initialized_(false), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors with higher precision
    alpha_ = 1.0 / static_cast<double>(params.period);
    alpha1_ = 1.0 - alpha_;
    
    _minperiod(params.period);
    
    // Add data source to datas for consistent data access
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
    }
}

SmoothedMovingAverage::SmoothedMovingAverage(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), prev_smma_(0.0), initialized_(false), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors with higher precision
    alpha_ = 1.0 / static_cast<double>(params.period);
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
    
    // Use the buffer directly for all access
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(smma_line);
    if (buffer) {
        // For performance tests with large datasets, the buffer might have
        // been filled via once() but the index wasn't properly set
        // Check actual data in buffer
        const auto& arr = buffer->array();
        size_t data_size = buffer->data_size();
        
        if (ago == 0) {
            // Get the current (last) value
            // When buffer has been filled by once(), idx might be set to last position
            // or it might be -1 (reset state)
            int idx = buffer->get_idx();
            
            // If idx is valid and points to the last element, use it
            if (idx >= 0 && idx == static_cast<int>(data_size) - 1 && idx < static_cast<int>(arr.size())) {
                return arr[idx];
            }
            // Otherwise, if we have data, return the last element
            else if (data_size > 0 && data_size <= arr.size()) {
                // Return the last calculated SMMA value
                return arr[data_size - 1];
            }
        } else if (ago < 0) {
            // Negative ago means access historical values
            // ago=0 is current (last), ago=-1 is one before last, etc.
            int target_idx = static_cast<int>(data_size) - 1 + ago;
            if (target_idx >= 0 && target_idx < static_cast<int>(data_size) && target_idx < static_cast<int>(arr.size())) {
                return arr[target_idx];
            }
        } else {
            // Positive ago means future values (not supported in batch mode)
            // For positive ago, we would need proper streaming with idx management
            return std::numeric_limits<double>::quiet_NaN();
        }
    }
    
    return std::numeric_limits<double>::quiet_NaN();
}


int SmoothedMovingAverage::getMinPeriod() const {
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
    
    // For buffers filled by once(), we need to return the data_size
    // because size() might return 0 if idx is -1
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(smma_line);
    if (buffer) {
        // If we have data, return the data size
        // This accounts for buffers filled via once() where idx might not reflect actual data
        size_t ds = buffer->data_size();
        if (ds > 0) {
            return ds;
        }
    }
    
    // Fallback to line size
    return smma_line->size();
}

void SmoothedMovingAverage::calculate() {
    // Get actual data size from the data lines
    size_t data_size = 0;
    
    if (!datas.empty() && datas[0]) {
        // For test framework compatibility, check the actual data loaded in buffer
        // The test framework resets line indices to -1, so size() returns 0
        
        // Get the appropriate data line
        std::shared_ptr<LineSingle> data_line;
        if (datas[0]->lines->size() >= 7) {
            // DataSeries with OHLCV - use close line (index 4)
            data_line = datas[0]->lines->getline(4);
        } else {
            // Single line data - use line 0
            data_line = datas[0]->lines->getline(0);
        }
        
        if (data_line) {
            // Check if it's a LineBuffer and get actual data size
            if (auto linebuf = std::dynamic_pointer_cast<LineBuffer>(data_line)) {
                // Use data_size() directly like SMA does
                // This gives us the total data loaded in the buffer
                data_size = linebuf->data_size();
            } else {
                // Fallback to size()
                data_size = data_line->size();
            }
        }
    } else if (data_source_) {
        // Check data_source_ similarly
        if (data_source_->lines && data_source_->lines->size() > 0) {
            std::shared_ptr<LineSingle> data_line;
            if (data_source_->lines->size() >= 7) {
                // DataSeries with OHLCV - use close line (index 4)
                data_line = data_source_->lines->getline(4);
            } else {
                // Single line data - use line 0
                data_line = data_source_->lines->getline(0);
            }
            
            if (data_line) {
                if (auto linebuf = std::dynamic_pointer_cast<LineBuffer>(data_line)) {
                    // Use data_size() directly like SMA does
                    data_size = linebuf->data_size();
                } else {
                    data_size = data_line->size();
                }
            }
        }
    }
    
    if (data_size > 0) {
        // Calculate SMMA for the entire dataset using once() method
        once(0, data_size);
    }
}

void SmoothedMovingAverage::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void SmoothedMovingAverage::prenext() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Use consistent data line selection logic like once() method
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() >= 7) {
        // DataSeries with OHLCV - use close line (index 4)
        data_line = datas[0]->lines->getline(4);
    } else {
        // Single line data - use line 0
        data_line = datas[0]->lines->getline(0);
    }
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
    
    // Use consistent data line selection logic like once() method
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() >= 7) {
        // DataSeries with OHLCV - use close line (index 4)
        data_line = datas[0]->lines->getline(4);
    } else {
        // Single line data - use line 0
        data_line = datas[0]->lines->getline(0);
    }
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
    
    // Get data line - check line count first
    if (!datas.empty() && datas[0] && datas[0]->lines) {
        if (datas[0]->lines->size() == 1) {
            // LineSeries with single line - use first line
            data_line = datas[0]->lines->getline(0);
        } else if (datas[0]->lines->size() >= 7) {
            // DataSeries with OHLCV - use close price at index 4
            // DataSeries has 7 lines: DateTime(0), Open(1), High(2), Low(3), Close(4), Volume(5), OpenInterest(6)
            data_line = datas[0]->lines->getline(4);
        } else if (datas[0]->lines->size() > 0) {
            // Other cases - use first line
            data_line = datas[0]->lines->getline(0);
        }
    } else if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        // Fallback to data_source_ if datas is empty
        if (data_source_->lines->size() == 1) {
            // LineSeries with single line - use first line
            data_line = data_source_->lines->getline(0);
        } else if (data_source_->lines->size() >= 7) {
            // DataSeries with OHLCV - use close price at index 4
            data_line = data_source_->lines->getline(4);
        } else if (data_source_->lines->size() > 0) {
            // Other cases - use first line
            data_line = data_source_->lines->getline(0);
        }
    }
    
    auto smma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(smma));
    if (!smma_line || !data_line) {
        return;
    }
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) {
        return;
    }
    
    // Get the actual data array and size
    const auto& data_array = data_buffer->array();
    
    // Use data_size() instead of array().size() as it represents the actual data loaded
    const size_t data_size = data_buffer->data_size();
    
    // Simplified SMMA calculation to exactly match Python logic
    // Python SMMA uses ExponentialSmoothing with alpha = 1/period
    // Initial value is calculated as SMA of first 'period' values
    
    smma_line->reset();  // Clear any existing data
    std::vector<double> smma_values;  // Store calculated SMMA values
    
    // Ensure we don't exceed array bounds
    const size_t array_size = data_array.size();
    const size_t actual_size = std::min(data_size, array_size);
    
    // Skip the initial NaN that might be in position 0
    size_t start_idx = 0;
    if (actual_size > 0 && std::isnan(data_array[0])) {
        start_idx = 1;
    }
    
    // Process data, considering start_idx offset
    for (size_t i = start_idx; i < actual_size; ++i) {
        size_t data_count = i - start_idx;  // Actual count of valid data points processed
        
        if (data_count < static_cast<size_t>(params.period - 1)) {
            // Not enough data for SMMA calculation
            smma_line->append(std::numeric_limits<double>::quiet_NaN());
        } else if (data_count == static_cast<size_t>(params.period - 1)) {
            // First SMMA value: calculate SMA of first 'period' values
            double sum = 0.0;
            bool all_valid = true;
            
            // Sum the first 'period' values starting from start_idx
            for (size_t j = start_idx; j < start_idx + params.period && j < array_size; ++j) {
                if (std::isnan(data_array[j])) {
                    all_valid = false;
                    break;
                }
                sum += data_array[j];
            }
            
            if (all_valid) {
                double first_smma = sum / params.period;
                smma_values.push_back(first_smma);
                smma_line->append(first_smma);
            } else {
                smma_values.push_back(std::numeric_limits<double>::quiet_NaN());
                smma_line->append(std::numeric_limits<double>::quiet_NaN());
            }
        } else {
            // Subsequent SMMA values: exponential smoothing
            // SMMA = prev_smma * (1 - alpha) + current_value * alpha
            // where alpha = 1/period
            if (!smma_values.empty() && !std::isnan(smma_values.back()) && 
                i < array_size && !std::isnan(data_array[i])) {
                double prev_smma = smma_values.back();
                double current_value = data_array[i];
                double new_smma = prev_smma * alpha1_ + current_value * alpha_;
                smma_values.push_back(new_smma);
                smma_line->append(new_smma);
            } else {
                smma_values.push_back(std::numeric_limits<double>::quiet_NaN());
                smma_line->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Set the LineBuffer index to the last element
    if (smma_line->array().size() > 0) {
        smma_line->set_idx(smma_line->array().size() - 1);
    }
}

} // namespace indicators
} // namespace backtrader