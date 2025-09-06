#include "indicators/williamsr.h"
#include "dataseries.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace backtrader {
namespace indicators {

WilliamsR::WilliamsR(int period) : Indicator(), using_line_roots_(false) {
    params.period = period;
    
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("percR", 0);
    }
}

WilliamsR::WilliamsR(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), using_line_roots_(false) {
    params.period = period;
    
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("percR", 0);
    }
    
    // Set data member for compatibility
    data = data_source_;
    this->datas.push_back(data_source_);
}

WilliamsR::WilliamsR(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), using_line_roots_(false) {
    params.period = period;
    
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("percR", 0);
    }
    
    // DataSeries inherits from LineSeries, so we can cast up
    auto lineseries = std::static_pointer_cast<LineSeries>(data_source);
    data_source_ = lineseries;
    this->data = lineseries;
    this->datas.push_back(lineseries);
}

std::vector<std::string> WilliamsR::_get_line_names() const {
    return {"percR"};
}

double WilliamsR::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!line_buffer) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use the LineBuffer's operator() which handles ago semantics correctly
    return (*line_buffer)[ago];
}

size_t WilliamsR::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto percr_line = lines->getline(0);
    return percr_line ? percr_line->size() : 0;
}

double WilliamsR::get_highest(int period, int start_ago) {
    // Always use data source directly
    double highest = std::numeric_limits<double>::lowest();
    bool found_valid_value = false;
    for (int i = start_ago; i < start_ago + period; ++i) {
        double high_val = datas[0]->high(i);
        if (!std::isnan(high_val)) {
            if (!found_valid_value || high_val > highest) {
                highest = high_val;
                found_valid_value = true;
            }
        }
    }
    // Return NaN if no valid values found
    return found_valid_value ? highest : std::numeric_limits<double>::quiet_NaN();
}

double WilliamsR::get_lowest(int period, int start_ago) {
    // Always use data source directly
    double lowest = std::numeric_limits<double>::max();
    bool found_valid_value = false;
    for (int i = start_ago; i < start_ago + period; ++i) {
        double low_val = datas[0]->low(i);
        if (!std::isnan(low_val)) {
            if (!found_valid_value || low_val < lowest) {
                lowest = low_val;
                found_valid_value = true;
            }
        }
    }
    // Return NaN if no valid values found
    return found_valid_value ? lowest : std::numeric_limits<double>::quiet_NaN();
}

double WilliamsR::get_highest_at_index(int period, int index) {
    // Always use data source directly
    double highest = std::numeric_limits<double>::lowest();
    for (int i = 0; i < period; ++i) {
        if (index - i >= 0) {
            double high_val = datas[0]->high(index - i);
            if (!std::isnan(high_val) && high_val > highest) {
                highest = high_val;
            }
        }
    }
    return highest;
}

double WilliamsR::get_lowest_at_index(int period, int index) {
    // Always use data source directly
    double lowest = std::numeric_limits<double>::max();
    for (int i = 0; i < period; ++i) {
        if (index - i >= 0) {
            double low_val = datas[0]->low(index - i);
            if (!std::isnan(low_val) && low_val < lowest) {
                lowest = low_val;
            }
        }
    }
    return lowest;
}

void WilliamsR::next() {
    if (!lines || lines->size() == 0) {
        return;
    }
    
    // Check if we have a data source - use same pattern as SMA
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    auto line_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!line_buffer) {
        return;
    }
    
    // Get OHLC lines - same pattern as SMA uses line index 4 for close
    auto high_line = data->lines->getline(2);   // High is line 2
    auto low_line = data->lines->getline(3);    // Low is line 3  
    auto close_line = data->lines->getline(4);  // Close is line 4
    
    if (!high_line || !low_line || !close_line) {
        line_buffer->append(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    
    if (!high_buffer || !low_buffer || !close_buffer) {
        line_buffer->append(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Get current index and check we have enough data
    int current_idx = close_buffer->get_idx();
    if (current_idx < params.period - 1) {
        line_buffer->append(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Get current close price - same method as SMA
    auto close_data = close_buffer->array();
    double current_close = close_data[current_idx];
    
    if (std::isnan(current_close)) {
        line_buffer->append(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate highest high and lowest low over the period
    auto high_data = high_buffer->array();
    auto low_data = low_buffer->array();
    
    double highest_high = std::numeric_limits<double>::lowest();
    double lowest_low = std::numeric_limits<double>::max();
    bool found_valid_values = false;
    
    // Look back over the period
    for (int i = 0; i < params.period; ++i) {
        int idx = current_idx - i;
        if (idx >= 0 && idx < static_cast<int>(high_data.size())) {
            double high_val = high_data[idx];
            double low_val = low_data[idx];
            
            if (!std::isnan(high_val) && !std::isnan(low_val)) {
                if (!found_valid_values || high_val > highest_high) {
                    highest_high = high_val;
                }
                if (!found_valid_values || low_val < lowest_low) {
                    lowest_low = low_val;
                }
                found_valid_values = true;
            }
        }
    }
    
    if (!found_valid_values) {
        line_buffer->append(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate Williams %R: %R = (Highest High - Close) / (Highest High - Lowest Low) * -100
    double williams_r;
    if (highest_high == lowest_low) {
        // When highest equals lowest, %R is undefined, typically set to -50
        williams_r = -50.0;
    } else {
        williams_r = ((highest_high - current_close) / (highest_high - lowest_low)) * -100.0;
        
        // Clamp to valid range [-100, 0] to avoid extreme values
        if (williams_r < -100.0) williams_r = -100.0;
        if (williams_r > 0.0) williams_r = 0.0;
    }
    
    line_buffer->append(williams_r);
}

void WilliamsR::calculate() {
    // Check if we have data
    if (!data || !data->lines || data->lines->size() < 4) {
        return;
    }
    
    // Get the close line to check data size
    std::shared_ptr<LineSingle> close_line;
    
    // Try standard DataSeries format first
    if (data->lines->size() >= 7) {
        close_line = data->lines->getline(DataSeries::Close);
    }
    
    // If that fails, try test format (no DateTime line)
    if (!close_line && data->lines->size() >= 4) {
        close_line = data->lines->getline(3); // close in test format
    }
    
    if (!close_line) {
        return;
    }
    
    // Check if we're in streaming mode by looking at the LineBuffer's current index
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    
    // Check current data size and buffer length
    size_t current_len = close_line->size();
    size_t buffer_len = 0;
    if (close_buffer) {
        buffer_len = close_buffer->buflen();  // Get actual array size
    }
    
    // Use batch mode (once()) if we have a lot of pre-filled data
    // Use the actual buffer length, not the size() which depends on index
    if (buffer_len > static_cast<size_t>(params.period * 2)) {
        // This is pre-filled data, use once() for batch processing
        once(0, static_cast<int>(buffer_len));
    } else {
        // For streaming mode or small data, use next()
        next();
    }
}

void WilliamsR::once(int start, int end) {
    if (!data || !data->lines || data->lines->size() < 4) {
        return;
    }
    
    
    // Try to get OHLC lines with flexible indexing
    std::shared_ptr<LineBuffer> high_buffer, low_buffer, close_buffer;
    
    // First try standard DataSeries indices
    if (data->lines->size() >= 7) {
        high_buffer = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(DataSeries::High));
        low_buffer = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(DataSeries::Low));
        close_buffer = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(DataSeries::Close));
    }
    
    // If that fails, try 5-line test format (Open, High, Low, Close, Volume - no DateTime/OpenInterest)
    if (!high_buffer || !low_buffer || !close_buffer) {
        if (data->lines->size() == 5) {
            // Test format: 0=Open, 1=High, 2=Low, 3=Close, 4=Volume
            high_buffer = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(1));  // high
            low_buffer = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(2));   // low
            close_buffer = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(3)); // close
        } else if (data->lines->size() >= 4) {
            // Fallback format
            high_buffer = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(1));  // high
            low_buffer = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(2));   // low
            close_buffer = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(3)); // close
        }
    }
    
    auto williams_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    
    if (!high_buffer || !low_buffer || !close_buffer || !williams_buffer) {
        return;
    }
    
    // Clear the buffer and reset
    williams_buffer->reset();
    
    // Get arrays for direct access
    const auto& high_array = high_buffer->array();
    const auto& low_array = low_buffer->array();
    const auto& close_array = close_buffer->array();
    
    // Calculate Williams %R for each position
    for (int i = start; i < end; ++i) {
        double williams_r = std::numeric_limits<double>::quiet_NaN();
        
        if (i >= params.period - 1) {
            // Get current close price at position i
            double current_close = (i < static_cast<int>(close_array.size())) ? close_array[i] : std::numeric_limits<double>::quiet_NaN();
            
            if (!std::isnan(current_close)) {
                // Find highest high and lowest low over the period
                double highest_high = std::numeric_limits<double>::lowest();
                double lowest_low = std::numeric_limits<double>::max();
                
                for (int j = 0; j < params.period; ++j) {
                    // Look back from current position over the period
                    // For position i, we want to look at positions i, i-1, i-2, ..., i-(period-1)
                    int idx = i - j;
                    if (idx >= 0 && idx < static_cast<int>(high_array.size())) {
                        double high_val = high_array[idx];
                        double low_val = low_array[idx];
                        
                        if (!std::isnan(high_val) && high_val > highest_high) {
                            highest_high = high_val;
                        }
                        if (!std::isnan(low_val) && low_val < lowest_low) {
                            lowest_low = low_val;
                        }
                    }
                }
                
                // Calculate Williams %R
                if (highest_high != lowest_low && highest_high != std::numeric_limits<double>::lowest()) {
                    williams_r = ((highest_high - current_close) / (highest_high - lowest_low)) * -100.0;
                    
                    // Clamp to valid range [-100, 0]
                    if (williams_r < -100.0) williams_r = -100.0;
                    if (williams_r > 0.0) williams_r = 0.0;
                    
                } else {
                    williams_r = 0.0;  // Avoid division by zero
                }
            }
        }
        
        williams_buffer->append(williams_r);
    }
    
    // After batch processing, set the buffer index to the last position
    // so that get(0) returns the last calculated value
    if (end > 0) {
        williams_buffer->set_idx(end - 1, true);
    }
}

} // namespace indicators
} // namespace backtrader