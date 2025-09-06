#include "indicators/percentrank.h"
#include <algorithm>
#include <iostream>
#include <limits>

namespace backtrader {
namespace indicators {

// PercentRank implementation
PercentRank::PercentRank() : Indicator(), data_source_(nullptr), current_index_(0) {
    std::cerr << "PctRank-ctor-default" << std::endl;
    setup_lines();
    _minperiod(params.period);
}

PercentRank::PercentRank(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    std::cout << "PCTRANK_CTOR_LS:" << period << std::endl;
    
    // Set up data and datas for compatibility with test framework
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
        std::cerr << "PctRank-ctor: data set" << std::endl;
    }
    
    // Reserve space for period data
    period_data_.reserve(params.period);
}

PercentRank::PercentRank(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    std::cerr << "PctRank-ctor-DataSeries: p=" << period << std::endl;
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // DataSeries is derived from LineSeries, so we can use it directly
    if (data_source) {
        this->data = std::static_pointer_cast<LineSeries>(data_source);
        this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
    }
    
    // Reserve space for period data
    period_data_.reserve(params.period);
}

double PercentRank::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(pctrank);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    if (buffer) {
        // For LineBuffer, we need to handle the ago value correctly
        // ago=0 means current position
        // ago < 0 means past values (e.g., -1 = previous value)
        return buffer->get(ago);
    }
    
    return (*line)[ago];
}

int PercentRank::getMinPeriod() const {
    return params.period;
}

size_t PercentRank::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto pctrank_line = lines->getline(pctrank);
    if (!pctrank_line) {
        return 0;
    }
    // Return the actual data size, not buffer size
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(pctrank_line);
    if (buffer) {
        return buffer->buflen();
    }
    return pctrank_line->size();
}

void PercentRank::calculate() {
    static int calc_count = 0;
    calc_count++;
    
    if (calc_count <= 5) {
        std::cerr << "PercentRank::calculate() #" << calc_count 
                  << " - data=" << (data ? "set" : "null") 
                  << ", data_source_=" << (data_source_ ? "set" : "null") << std::endl;
    }
    
    if (data && data->lines && data->lines->size() > 0) {
        // For data set via data member (used by tests)
        // Use batch processing approach like SMA
        
        if (calc_count <= 5) {
            std::cerr << "PercentRank::calculate() #" << calc_count 
                      << " - using data path, lines_size=" << data->lines->size() << std::endl;
        }
        
        // Use correct line based on data type: Close is line 4 for DataSeries, line 0 for LineSeries
        auto data_line = data->lines->getline(data->lines->size() > 4 ? 4 : 0);
        
        if (!data_line) {
            if (calc_count <= 5) {
                std::cerr << "PercentRank::calculate() #" << calc_count 
                          << " - no data_line!" << std::endl;
            }
            return;
        }
        
        auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
        if (!data_buffer) {
            if (calc_count <= 5) {
                std::cerr << "PercentRank::calculate() #" << calc_count 
                          << " - no data_buffer!" << std::endl;
            }
            return;
        }
        
        auto pctrank_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(pctrank));
        if (!pctrank_line) {
            if (calc_count <= 5) {
                std::cerr << "PercentRank::calculate() #" << calc_count 
                          << " - no pctrank_line!" << std::endl;
            }
            return;
        }
        
        if (calc_count <= 5) {
            std::cerr << "PercentRank::calculate() #" << calc_count 
                      << " - data_buffer_size=" << data_buffer->data_size()
                      << ", pctrank_size=" << pctrank_line->size() << std::endl;
        }
        
        // Check if this is the first call - if so, do batch processing
        // LineBuffer starts with one NaN value, so check if size is <= 1
        if (pctrank_line->size() <= 1) {
            if (calc_count <= 5) {
                std::cerr << "PercentRank::calculate() #" << calc_count 
                          << " - calling once() for batch processing" << std::endl;
            }
            // Batch process all data using once() method
            // Pass the actual data size
            once(0, static_cast<int>(data_buffer->data_size()));
        } else {
            // If already calculated, sync buffer position with data buffer
            int data_idx = data_buffer->get_idx();
            if (data_idx >= 0 && data_idx < static_cast<int>(pctrank_line->size())) {
                pctrank_line->set_idx(data_idx);
            }
        }
        
        if (calc_count <= 5) {
            std::cerr << "PercentRank::calculate() #" << calc_count 
                      << " - after batch processing, pctrank_size=" << pctrank_line->size() << std::endl;
        }
        
    } else if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        // For streaming mode with data_source_
        if (calc_count <= 5) {
            std::cerr << "PercentRank::calculate() #" << calc_count 
                      << " - using data_source_ path" << std::endl;
        }
        
        // Use correct line based on data type: Close is line 4 for DataSeries, line 0 for LineSeries
        auto data_line = data_source_->lines->getline(data_source_->lines->size() > 4 ? 4 : 0);
        
        if (!data_line) {
            return;
        }
        
        // Check current state like SMA does
        size_t current_len = data_line->size();
        
        if (calc_count <= 5) {
            std::cerr << "PercentRank::calculate() (streaming) #" << calc_count 
                      << " - current_len=" << current_len << ", minperiod_=" << minperiod_ << std::endl;
        }
        
        if (current_len >= minperiod_) {
            next();
        } else if (current_len > 0) {
            prenext();
        }
    } else {
        if (calc_count <= 5) {
            std::cerr << "PercentRank::calculate() #" << calc_count 
                      << " - no valid data source!" << std::endl;
        }
    }
}

void PercentRank::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("pctrank", 0);
    }
}

void PercentRank::prenext() {
    // During warm-up period, append NaN to output buffer
    auto pctrank_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(pctrank));
    if (pctrank_line) {
        if (pctrank_line->size() == 0) {
            pctrank_line->set(0, std::numeric_limits<double>::quiet_NaN());
        } else {
            pctrank_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Collect data for sliding window but don't calculate percentrank yet
    std::shared_ptr<LineSeries> data_to_use;
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        data_to_use = data_source_;
    } else if (data && data->lines && data->lines->size() > 0) {
        data_to_use = data;
    } else {
        return;
    }
    
    // Use correct line based on data type: Close is line 4 for DataSeries, line 0 for LineSeries
    auto data_line = data_to_use->lines->getline(data_to_use->lines->size() > 4 ? 4 : 0);
    
    if (!data_line) {
        return;
    }
    
    // CRITICAL FIX: Use the same data access method as SMA
    // The issue is that (*data_line)[0] doesn't work properly in streaming mode
    double current_value = std::numeric_limits<double>::quiet_NaN();
    
    if (auto line_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line)) {
        int current_idx = line_buffer->get_idx();
        auto raw_data = line_buffer->array();
        
        // If index is valid and within bounds, get the value directly from array
        if (current_idx >= 0 && current_idx < static_cast<int>(raw_data.size())) {
            current_value = raw_data[current_idx];
        } else {
            // Try alternative access methods
            double direct_access = (*data_line)[0];
            
            if (!std::isnan(direct_access)) {
                current_value = direct_access;
            }
        }
    } else {
        // Fallback for non-LineBuffer cases
        current_value = (*data_line)[0];
    }
    
    // Debug output for first few calls
    static int prenext_count = 0;
    prenext_count++;
    if (prenext_count <= 10) {
        std::cerr << "PercentRank::prenext() #" << prenext_count 
                  << " - current_value=" << current_value 
                  << ", period_data_.size()=" << period_data_.size() << std::endl;
    }
    
    if (!std::isnan(current_value)) {
        // Add to sliding window
        period_data_.push_back(current_value);
        
        // Keep only the period we need
        if (period_data_.size() > static_cast<size_t>(params.period)) {
            period_data_.erase(period_data_.begin());
        }
    }
}

void PercentRank::next() {
    // Get data source
    std::shared_ptr<LineSeries> data_to_use;
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        data_to_use = data_source_;
    } else if (data && data->lines && data->lines->size() > 0) {
        data_to_use = data;
    } else {
        return;
    }
    
    // Use correct line based on data type: Close is line 4 for DataSeries, line 0 for LineSeries
    auto data_line = data_to_use->lines->getline(data_to_use->lines->size() > 4 ? 4 : 0);
    
    auto pctrank_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(pctrank));
    
    if (!data_line || !pctrank_line) return;
    
    // CRITICAL FIX: Use the same data access method as SMA
    double current_value = std::numeric_limits<double>::quiet_NaN();
    
    if (auto line_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line)) {
        int current_idx = line_buffer->get_idx();
        auto raw_data = line_buffer->array();
        
        // If index is valid and within bounds, get the value directly from array
        if (current_idx >= 0 && current_idx < static_cast<int>(raw_data.size())) {
            current_value = raw_data[current_idx];
        } else {
            // Try alternative access methods
            double direct_access = (*data_line)[0];
            
            if (!std::isnan(direct_access)) {
                current_value = direct_access;
            }
        }
    } else {
        // Fallback for non-LineBuffer cases
        current_value = (*data_line)[0];
    }
    
    // Debug output for first few calls
    static int next_count = 0;
    next_count++;
    if (next_count <= 10) {
        std::cerr << "PercentRank::next() #" << next_count 
                  << " - current_value=" << current_value 
                  << ", period_data_.size()=" << period_data_.size() << std::endl;
    }
    
    if (!std::isnan(current_value)) {
        // Add to sliding window
        period_data_.push_back(current_value);
        
        // Keep only the period we need
        if (period_data_.size() > static_cast<size_t>(params.period)) {
            period_data_.erase(period_data_.begin());
        }
        
        // Calculate percent rank - we should have enough data since next() is called
        if (period_data_.size() >= static_cast<size_t>(params.period)) {
            double pct_rank = calculate_percent_rank(period_data_, current_value);
            pctrank_line->append(pct_rank);
        } else {
            // This shouldn't happen in next(), but handle it gracefully
            pctrank_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    } else {
        // If current value is NaN, append NaN
        pctrank_line->append(std::numeric_limits<double>::quiet_NaN());
    }
}

void PercentRank::once(int start, int end) {
    // Handle both data and data_source_
    std::shared_ptr<LineSeries> data_to_use;
    if (data && data->lines && data->lines->size() > 0) {
        data_to_use = data;
    } else if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        data_to_use = data_source_;
    } else {
        return;
    }
    
    // Use correct line based on data type: Close is line 4 for DataSeries, line 0 for LineSeries
    auto data_line = data_to_use->lines->getline(data_to_use->lines->size() > 4 ? 4 : 0);
    auto pctrank_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(pctrank));
    if (!pctrank_line || !data_line) {
        return;
    }
    
    // Get data buffer to access underlying array
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) {
        return;
    }
    
    // Clear the buffer completely to start fresh
    pctrank_line->clear();
    
    static int once_count = 0;
    once_count++;
    if (once_count <= 3) {
        std::cerr << "PercentRank::once() #" << once_count 
                  << " - start=" << start << ", end=" << end
                  << ", period=" << params.period << std::endl;
    }
    
    // Access the underlying data array directly
    const auto& raw_array = data_buffer->array();
    
    // Process data from start to end (end is the size, not the last index)
    for (int i = start; i < end; ++i) {
        double percent_rank_val = std::numeric_limits<double>::quiet_NaN();
        
        // Check if we have enough data points for calculation
        // Need at least period points (i starts from 0, so i >= period-1)
        if (i >= params.period - 1) {
            // Calculate PercentRank for position i using the window [i-period+1, i]
            std::vector<double> window_data;
            
            // Collect the window data (period data points ending at position i)
            for (int j = i - params.period + 1; j <= i; ++j) {
                if (j >= 0 && j < end) {
                    double value = raw_array[j];
                    if (!std::isnan(value)) {
                        window_data.push_back(value);
                    }
                }
            }
            
            // Calculate percent rank if we have exactly the right amount of data
            if (static_cast<int>(window_data.size()) == params.period) {
                // Get current value (at position i)
                double current_value = raw_array[i];
                
                if (!std::isnan(current_value)) {
                    percent_rank_val = calculate_percent_rank(window_data, current_value);
                    
                    if (once_count <= 3 && i < 60) {  // Debug first few calculations
                        std::cerr << "PercentRank::once() #" << once_count 
                                  << " i=" << i << " current_value=" << current_value 
                                  << " pct_rank=" << percent_rank_val << std::endl;
                    }
                }
            }
        }
        
        // Use append to add value to buffer
        pctrank_line->append(percent_rank_val);
    }
    
    // Set buffer index to the end position for proper get() access
    if (pctrank_line->size() > 0) {
        auto pctrank_buffer = std::dynamic_pointer_cast<LineBuffer>(pctrank_line);
        if (pctrank_buffer) {
            pctrank_buffer->set_idx(pctrank_buffer->size() - 1);
        }
    }
    
    if (once_count <= 3) {
        std::cerr << "PercentRank::once() #" << once_count 
                  << " - completed, pctrank_size=" << pctrank_line->size() << std::endl;
    }
}

double PercentRank::calculate_percent_rank(const std::vector<double>& data, double current_value) {
    if (data.empty()) return 0.0;
    
    // Count how many values are less than the current value
    // Following Python's implementation: fsum(x < d[-1] for x in d) / len(d)
    int count_less = 0;
    for (double value : data) {
        if (value < current_value) {
            count_less++;
        }
    }
    
    // Percent rank = count of values less than current / total count
    // This matches Python: fsum(x < d[-1] for x in d) / len(d)
    return static_cast<double>(count_less) / data.size();
}
} // namespace indicators
} // namespace backtrader