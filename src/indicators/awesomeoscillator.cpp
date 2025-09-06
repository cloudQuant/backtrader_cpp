#include "indicators/awesomeoscillator.h"
#include "lineseries.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace backtrader {

// AwesomeOscillator implementation
AwesomeOscillator::AwesomeOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.slow);
}

AwesomeOscillator::AwesomeOscillator(std::shared_ptr<LineSeries> data_source) : Indicator() {
    setup_lines();
    _minperiod(params.slow);
    
    // This constructor is for test framework compatibility
    // Set the data member for the calculate method
    data = data_source;
    datas.push_back(data_source);
}

AwesomeOscillator::AwesomeOscillator(std::shared_ptr<DataSeries> data_source) : Indicator() {
    setup_lines();
    _minperiod(params.slow);
    
    // This constructor is for test framework compatibility
    // Set the data member for the calculate method
    data = data_source;
    datas.push_back(data_source);
}

AwesomeOscillator::AwesomeOscillator(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low) : Indicator() {
    setup_lines();
    _minperiod(params.slow);
    
    // Store the high and low line data directly
    high_data_ = high;
    low_data_ = low;
    
    // Use high as the primary data source for compatibility
    data = high;
    datas.push_back(high);
}

void AwesomeOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("ao", 0);
    }
    
    // Connect LineCollection to IndicatorBase lines_ vector for size() method
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
}

double AwesomeOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto ao_line = lines->getline(ao);
    if (!ao_line || ao_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // LineBuffer already correctly implements Python-style indexing
    // Just pass the ago value directly
    return (*ao_line)[ago];
}

int AwesomeOscillator::getMinPeriod() const {
    return params.slow;
}

size_t AwesomeOscillator::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto ao_line = lines->getline(ao);
    if (!ao_line) {
        return 0;
    }
    return ao_line->size();
}

void AwesomeOscillator::prenext() {
    next();
}

void AwesomeOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    auto low_line = datas[0]->lines->getline(2);  // Low line
    auto ao_line = lines->getline(ao);
    
    if (!high_line || !low_line || !ao_line) return;
    
    // Calculate median price = (high + low) / 2
    double high_value = (*high_line)[0];
    double low_value = (*low_line)[0];
    double median_price = (high_value + low_value) / 2.0;
    
    // Add to median price history
    median_prices_.push_back(median_price);
    
    // Keep only what we need for the longest SMA
    if (median_prices_.size() > params.slow * 2) {
        median_prices_.erase(median_prices_.begin());
    }
    
    // Need enough data for both SMAs
    if (median_prices_.size() < params.slow) {
        ao_line->set(0, std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate SMA fast (last 5 values)
    double sum_fast = 0.0;
    for (int i = 0; i < params.fast; ++i) {
        sum_fast += median_prices_[median_prices_.size() - 1 - i];
    }
    double sma_fast = sum_fast / params.fast;
    
    // Calculate SMA slow (last 34 values)
    double sum_slow = 0.0;
    for (int i = 0; i < params.slow; ++i) {
        sum_slow += median_prices_[median_prices_.size() - 1 - i];
    }
    double sma_slow = sum_slow / params.slow;
    
    // AO = SMA(fast) - SMA(slow)
    ao_line->set(0, sma_fast - sma_slow);
}

void AwesomeOscillator::once(int start, int end) {
    std::shared_ptr<LineSingle> high_line, low_line;
    
    // Use direct high/low data if available (LineSeries constructor)
    if (high_data_ && low_data_) {
        high_line = high_data_->lines->getline(0);
        low_line = low_data_->lines->getline(0);
    } else {
        // Fallback to combined data approach
        if (datas.empty() || !datas[0]->lines) return;
        high_line = datas[0]->lines->getline(2);  // High is at index 2
        low_line = datas[0]->lines->getline(3);   // Low is at index 3
    }
    
    auto ao_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ao));
    
    if (!high_line || !low_line || !ao_line) return;
    
    // Cast to LineBuffer to get buflen and data_ptr
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    
    if (!high_buffer || !low_buffer) return;
    
    // Use the actual data size
    int raw_size = static_cast<int>(std::min(high_buffer->buflen(), low_buffer->buflen()));
    
    // Get direct access to data arrays
    const double* high_data = high_buffer->data_ptr();
    const double* low_data = low_buffer->data_ptr();
    
    // Handle LineBuffer's initial NaN value
    // The test data has 255 real values plus 1 initial NaN = 256 total
    // But we need to include the NaN in our output to match Python's indexing
    int data_size = raw_size;  // Include the initial NaN for proper indexing
    int start_idx = 0;  // Don't skip the initial NaN
    
    // Clear the LineBuffer and prepare for new data
    ao_line->reset();
    
    // Calculate AO values in chronological order (oldest to newest)
    // This matches how the test data is loaded with batch_append()
    for (int i = 0; i < data_size; ++i) {
        double ao_value;
        
        // First value is always NaN (initial buffer state)
        if (i == 0 || std::isnan(high_data[i]) || std::isnan(low_data[i])) {
            ao_value = std::numeric_limits<double>::quiet_NaN();
        } else if (i < params.slow) {  // Need 34 valid values for calculation
            // Insufficient data for calculation
            ao_value = std::numeric_limits<double>::quiet_NaN();
        } else {
            // Calculate median prices and SMAs for the current position
            double sum_fast = 0.0;
            double sum_slow = 0.0;
            int valid_fast = 0;
            int valid_slow = 0;
            
            // Accumulate values for SMAs going backwards from current position
            for (int j = 0; j < params.slow; ++j) {
                int idx = i - j;  // Look back from current position
                if (idx > 0 && idx < raw_size) {  // Skip index 0 (initial NaN)
                    double high_val = high_data[idx];
                    double low_val = low_data[idx];
                    if (!std::isnan(high_val) && !std::isnan(low_val)) {
                        double median_price = (high_val + low_val) / 2.0;
                        
                        if (j < params.fast) {
                            sum_fast += median_price;
                            valid_fast++;
                        }
                        sum_slow += median_price;
                        valid_slow++;
                    }
                }
            }
            
            // Only calculate if we have enough valid values
            if (valid_fast == params.fast && valid_slow == params.slow) {
                double sma_fast = sum_fast / params.fast;
                double sma_slow = sum_slow / params.slow;
                ao_value = sma_fast - sma_slow;
            } else {
                ao_value = std::numeric_limits<double>::quiet_NaN();
            }
        }
        
        // Append the calculated value
        ao_line->append(ao_value);
    }
}

void AwesomeOscillator::calculate() {
    std::shared_ptr<LineSingle> high_line, low_line;
    
    // Use direct high/low data if available (LineSeries constructor)
    if (high_data_ && low_data_) {
        high_line = high_data_->lines->getline(0);
        low_line = low_data_->lines->getline(0);
    } else {
        // Fallback to combined data approach
        if (datas.empty() || !datas[0]->lines) {
            std::cerr << "AwesomeOscillator::calculate() - No data available" << std::endl;
            return;
        }
        
        // Debug: Check available lines
        // std::cerr << "AwesomeOscillator::calculate() - Available lines: " << datas[0]->lines->size() << std::endl;
        
        // Correct OHLCV indexing: 0=DateTime, 1=Open, 2=High, 3=Low, 4=Close, 5=Volume, 6=OpenInterest
        high_line = datas[0]->lines->getline(2);  // High is at index 2
        low_line = datas[0]->lines->getline(3);   // Low is at index 3
    }
    
    if (!high_line || !low_line) {
        std::cerr << "AwesomeOscillator::calculate() - High or low line not found" << std::endl;
        return;
    }
    
    auto ao_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ao));
    if (!ao_line) return;
    
    // Get current data size - use buflen() to get actual data, not current position
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    
    if (!high_buffer || !low_buffer) {
        std::cerr << "AwesomeOscillator::calculate() - High or low not LineBuffer" << std::endl;
        return;
    }
    
    int raw_size = static_cast<int>(std::min(high_buffer->buflen(), low_buffer->buflen()));
    
    // std::cerr << "AwesomeOscillator::calculate() - Data size (buflen): " << raw_size << std::endl;
    // std::cerr << "AwesomeOscillator::calculate() - AO line size before: " << ao_line->size() << std::endl;
    
    // Get direct access to data arrays
    const double* high_data = high_buffer->data_ptr();
    const double* low_data = low_buffer->data_ptr();
    
    // Handle LineBuffer's initial NaN value
    // The test data has 255 real values plus 1 initial NaN = 256 total
    // But we need to include the NaN in our output to match Python's indexing
    int data_size = raw_size;  // Include the initial NaN for proper indexing
    int start_idx = 0;  // Don't skip the initial NaN
    
    // Debug: Check first few high/low values
    // std::cerr << "First few high/low values:" << std::endl;
    // for (int i = start_idx; i < std::min(start_idx + 5, raw_size); ++i) {
    //     std::cerr << "  [" << i << "] High=" << high_data[i] << ", Low=" << low_data[i] 
    //               << ", Median=" << (high_data[i] + low_data[i]) / 2.0 << std::endl;
    // }
    
    // If this is the first calculation or data size has changed significantly, recalculate all
    if (ao_line->size() == 0 || std::abs(static_cast<int>(ao_line->size()) - data_size) > 1) {
        // Debug: Check first few high/low values
        std::cerr << "AO Calculate debug:" << std::endl;
        std::cerr << "  raw_size=" << raw_size << ", data_size=" << data_size << ", start_idx=" << start_idx << std::endl;
        std::cerr << "  AO line size before reset: " << ao_line->size() << std::endl;
        
        // Clear and recalculate for entire dataset
        ao_line->reset();
        std::cerr << "  AO line size after reset: " << ao_line->size() << std::endl;
        
        // Calculate AO values in chronological order (oldest to newest)
        for (int i = 0; i < data_size; ++i) {
            double ao_value;
            
            // First value is always NaN (initial buffer state)
            if (i == 0 || std::isnan(high_data[i]) || std::isnan(low_data[i])) {
                ao_value = std::numeric_limits<double>::quiet_NaN();
            } else if (i < params.slow) {  // Need at least 34 valid values
                // At i=33, looking back 34 positions gives indices 0-33 (but 0 is NaN)
                // At i=34, looking back 34 positions gives indices 1-34 (34 valid values)
                ao_value = std::numeric_limits<double>::quiet_NaN();
            } else {
                // Calculate median prices and SMAs for the current position
                double sum_fast = 0.0;
                double sum_slow = 0.0;
                int valid_fast = 0;
                int valid_slow = 0;
                
                // Debug: Print calculation for first AO value
                bool debug_first = (i == params.slow);
                if (debug_first) {
                    std::cerr << "  Calculating first AO at i=" << i << std::endl;
                }
                
                // Accumulate values for SMAs going backwards from current position
                // For position i in the data (0-based), we look back j positions
                for (int j = 0; j < params.slow; ++j) {
                    int idx = i - j;  // Look back from current position
                    if (idx > 0 && idx < raw_size) {  // Skip index 0 (initial NaN)
                        // Access raw array data directly
                        double high_val = high_data[idx];
                        double low_val = low_data[idx];
                        if (!std::isnan(high_val) && !std::isnan(low_val)) {
                            double median_price = (high_val + low_val) / 2.0;
                            
                            if (debug_first && j < 5) {
                                std::cerr << "    j=" << j << ", idx=" << idx 
                                          << ", high=" << high_val << ", low=" << low_val
                                          << ", median=" << median_price << std::endl;
                            }
                            
                            // Add to fast SMA sum (most recent 5 values)
                            if (j < params.fast) {
                                sum_fast += median_price;
                                valid_fast++;
                            }
                            // Add to slow SMA sum (most recent 34 values)
                            sum_slow += median_price;
                            valid_slow++;
                        }
                    }
                }
                
                // Only calculate if we have enough valid values
                if (valid_fast == params.fast && valid_slow == params.slow) {
                    double sma_fast = sum_fast / params.fast;
                    double sma_slow = sum_slow / params.slow;
                    ao_value = sma_fast - sma_slow;
                } else {
                    ao_value = std::numeric_limits<double>::quiet_NaN();
                }
            }
            
            ao_line->append(ao_value);
        }
    } else {
        // Incremental calculation for the last value only
        int i = data_size - 1;
        double ao_value;
        
        if (i < params.slow - 1) {
            ao_value = std::numeric_limits<double>::quiet_NaN();
        } else {
            double sum_fast = 0.0;
            double sum_slow = 0.0;
            
            // Calculate SMAs looking back from current position
            for (int j = 0; j < params.slow; ++j) {
                int idx = i - j + start_idx; // Adjust for potential NaN skip
                if (idx >= start_idx && idx < raw_size) {
                    double high_val = high_data[idx];
                    double low_val = low_data[idx];
                    double median_price = (high_val + low_val) / 2.0;
                    
                    if (j < params.fast) {
                        sum_fast += median_price;
                    }
                    sum_slow += median_price;
                }
            }
            
            double sma_fast = sum_fast / params.fast;
            double sma_slow = sum_slow / params.slow;
            ao_value = sma_fast - sma_slow;
        }
        
        // Update only the last value
        if (ao_line->size() < data_size) {
            ao_line->append(ao_value);
        } else {
            ao_line->set(0, ao_value);  // Update the most recent value
        }
    }
}

} // namespace backtrader