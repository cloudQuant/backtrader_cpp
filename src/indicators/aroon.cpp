#include "indicators/aroon.h"
#include "dataseries.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>

namespace backtrader {


// AroonBase implementation
AroonBase::AroonBase(bool calc_up, bool calc_down) 
    : Indicator(), calc_up_(calc_up), calc_down_(calc_down), up_value_(0.0), down_value_(0.0) {
    params.period = 14; // Default period
    _minperiod(params.period + 1);
}

void AroonBase::calculate() {
    if (!lines || lines->size() == 0) {
        setup_lines();
    }
    
    // Run calculate_lines for actual calculation
    calculate_lines();
}

void AroonBase::prenext() {
    // Not enough data yet, set NaN values
    int position = lines->getline(0)->size();
    set_nan_values_at_position(position);
}

void AroonBase::next() {
    // Calculate the indicator values
    calculate_lines();
}

void AroonBase::once(int start, int end) {
    // Batch calculation
    for (int i = start; i < end; ++i) {
        calculate_lines_at_position(i);
    }
}

int AroonBase::find_highest_index(int period) {
    if (datas.empty() || !datas[0]->lines) return 0;
    
    auto high_line = datas[0]->lines->getline(2); // High is line 2 in SimpleTestDataSeries
    if (!high_line || high_line->size() < period) return 0;
    
    double highest = std::numeric_limits<double>::lowest();
    int highest_idx = 0;
    
    for (int i = 0; i < period; ++i) {
        double val = (*high_line)[i];
        if (val > highest) {
            highest = val;
            highest_idx = i;
        }
    }
    
    return highest_idx;
}

int AroonBase::find_lowest_index(int period) {
    if (datas.empty() || !datas[0]->lines) return 0;
    
    auto low_line = datas[0]->lines->getline(3); // Low is line 3 in SimpleTestDataSeries
    if (!low_line || low_line->size() < period) return 0;
    
    double lowest = std::numeric_limits<double>::max();
    int lowest_idx = 0;
    
    for (int i = 0; i < period; ++i) {
        double val = (*low_line)[i];
        if (val < lowest) {
            lowest = val;
            lowest_idx = i;
        }
    }
    
    return lowest_idx;
}

int AroonBase::find_highest_index_at_position(int position, int period) {
    if (datas.empty() || !datas[0]->lines) return 0;
    
    auto high_line = datas[0]->lines->getline(2); // High is line 2 in SimpleTestDataSeries
    if (!high_line || position < period - 1) return 0;
    
    double highest = std::numeric_limits<double>::lowest();
    int highest_idx = 0;
    
    for (int i = 0; i < period; ++i) {
        int data_idx = position - i;
        if (data_idx < 0) break;
        
        double val = (*high_line)[data_idx];
        if (val > highest) {
            highest = val;
            highest_idx = i;
        }
    }
    
    return highest_idx;
}

int AroonBase::find_lowest_index_at_position(int position, int period) {
    if (datas.empty() || !datas[0]->lines) return 0;
    
    auto low_line = datas[0]->lines->getline(3); // Low is line 3 in SimpleTestDataSeries
    if (!low_line || position < period - 1) return 0;
    
    double lowest = std::numeric_limits<double>::max();
    int lowest_idx = 0;
    
    for (int i = 0; i < period; ++i) {
        int data_idx = position - i;
        if (data_idx < 0) break;
        
        double val = (*low_line)[data_idx];
        if (val < lowest) {
            lowest = val;
            lowest_idx = i;
        }
    }
    
    return lowest_idx;
}

void AroonBase::calculate_lines_at_position(int position) {
    // Default implementation - derived classes should override if needed
    if (position < params.period - 1) {
        set_nan_values_at_position(position);
        return;
    }
    
    // Calculate AroonUp and AroonDown values at the given position
    if (calc_up_) {
        int highest_idx = find_highest_index_at_position(position, params.period);
        up_value_ = 100.0 * (params.period - 1 - highest_idx) / (params.period - 1);
        if (lines && lines->size() > 0) {
            auto up_line = lines->getline(0);
            if (up_line && position < up_line->size()) {
                up_line->set(position, up_value_);
            }
        }
    }
    
    if (calc_down_) {
        int lowest_idx = find_lowest_index_at_position(position, params.period);
        down_value_ = 100.0 * (params.period - 1 - lowest_idx) / (params.period - 1);
        if (lines && lines->size() > 1) {
            auto down_line = lines->getline(1);
            if (down_line && position < down_line->size()) {
                down_line->set(position, down_value_);
            }
        }
    }
}

void AroonBase::set_nan_values_at_position(int position) {
    if (!lines) return;
    
    // Set NaN values for all lines at the given position
    for (size_t i = 0; i < lines->size(); ++i) {
        auto line = lines->getline(i);
        if (line) {
            // If line is a LineBuffer, we can append
            auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
            if (buffer && buffer->size() == position) {
                buffer->append(std::numeric_limits<double>::quiet_NaN());
            } else if (position < line->size()) {
                line->set(position, std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
}

// AroonUp implementation
AroonUp::AroonUp() : AroonBase(true, false) {
    setup_lines();
}

void AroonUp::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AroonUp::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto aroonup_line = lines->getline(aroonup);
    if (!aroonup_line) return;
    
    // Find index of highest high in period + 1 bars
    int highest_idx = find_highest_index(params.period + 1);
    
    // Calculate AroonUp: 100 * (period - distance to highest high) / period
    up_value_ = (100.0 / params.period) * (params.period - highest_idx);
    aroonup_line->set(0, up_value_);
}

double AroonUp::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto aroonup_line = lines->getline(aroonup);
    if (!aroonup_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*aroonup_line)[ago];
}

int AroonUp::getMinPeriod() const {
    return params.period;
}

// AroonDown implementation
AroonDown::AroonDown() : AroonBase(false, true) {
    setup_lines();
}

void AroonDown::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AroonDown::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto aroondown_line = lines->getline(aroondown);
    if (!aroondown_line) return;
    
    // Find index of lowest low in period + 1 bars
    int lowest_idx = find_lowest_index(params.period + 1);
    
    // Calculate AroonDown: 100 * (period - distance to lowest low) / period
    down_value_ = (100.0 / params.period) * (params.period - lowest_idx);
    aroondown_line->set(0, down_value_);
}

double AroonDown::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto aroondown_line = lines->getline(aroondown);
    if (!aroondown_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*aroondown_line)[ago];
}

int AroonDown::getMinPeriod() const {
    return params.period;
}

// AroonUpDown implementation  
AroonUpDown::AroonUpDown() : AroonBase(true, true) {
    setup_lines();
}

AroonUpDown::AroonUpDown(std::shared_ptr<DataSeries> data_source, int period) : AroonBase(true, true) {
    params.period = period;
    setup_lines();
    _minperiod(period + 1);
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

AroonUpDown::AroonUpDown(std::shared_ptr<LineSeries> data_source, int period) : AroonBase(true, true) {
    params.period = period;
    setup_lines();
    _minperiod(period + 1);
    
    // For single LineSeries, assume it's OHLC data and extract high/low from it
    this->data = data_source;
    this->datas.push_back(data_source);
}

AroonUpDown::AroonUpDown(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low, int period) : AroonBase(true, true) {
    params.period = period;
    setup_lines();
    _minperiod(period + 1);
    
    // Store LineSeries sources
    this->data = high;
    this->datas.push_back(high);
    this->datas.push_back(low);
}

void AroonUpDown::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("aroonup", 0);
        lines->add_alias("aroondown", 1);
    }
}

double AroonUpDown::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto aroonup_line = lines->getline(0);
    if (!aroonup_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // LineBuffer already correctly implements Python-style indexing
    // Just pass the ago value directly
    return (*aroonup_line)[ago];
}

double AroonUpDown::getAroonUp(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto aroonup_line = lines->getline(0);
    if (!aroonup_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // LineBuffer already correctly implements Python-style indexing
    // Just pass the ago value directly
    return (*aroonup_line)[ago];
}

double AroonUpDown::getAroonDown(int ago) const {
    if (!lines || lines->size() < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto aroondown_line = lines->getline(1);
    if (!aroondown_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // LineBuffer already correctly implements Python-style indexing
    // Just pass the ago value directly
    return (*aroondown_line)[ago];
}

int AroonUpDown::getMinPeriod() const {
    return params.period + 1;
}

size_t AroonUpDown::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto aroonup_line = lines->getline(0);
    if (!aroonup_line) {
        return 0;
    }
    return aroonup_line->size();
}

void AroonUpDown::calculate() {
    if (datas.empty()) {
        return;
    }
    
    // Handle single OHLCV data source or separate high/low data sources
    std::shared_ptr<LineSingle> high_line;
    std::shared_ptr<LineSingle> low_line;
    
    if (datas.size() == 1) {
        // Single OHLCV data source - extract high and low lines
        auto data_source = datas[0];
        if (!data_source || !data_source->lines || data_source->lines->size() < 5) {
            return;
        }
        
        // For OHLCV: High is line 2, Low is line 3
        high_line = data_source->lines->getline(2);
        low_line = data_source->lines->getline(3);
    } else if (datas.size() >= 2) {
        // Separate high and low data sources
        auto high_data = datas[0];
        auto low_data = datas[1];
        
        if (!high_data || !high_data->lines || high_data->lines->size() == 0 ||
            !low_data || !low_data->lines || low_data->lines->size() == 0) {
            return;
        }
        
        high_line = high_data->lines->getline(0);
        low_line = low_data->lines->getline(0);
    } else {
        return;
    }
    
    if (!high_line || !low_line) return;
    
    auto aroonup_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    auto aroondown_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(1));
    if (!aroonup_line || !aroondown_line) return;
    
    // Get the data buffers
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    if (!high_buffer || !low_buffer) return;
    
    // Clear and recalculate all values
    aroonup_line->reset();
    aroondown_line->reset();
    
    const auto& high_array = high_buffer->array();
    const auto& low_array = low_buffer->array();
    int data_size = static_cast<int>(high_array.size());
    
    // Skip initial NaN if present (LineBuffer starts with NaN at index 0)
    int start_idx = 0;
    if (data_size > 0 && std::isnan(high_array[0])) {
        start_idx = 1;
        // Add initial NaN to match data structure
        aroonup_line->append(std::numeric_limits<double>::quiet_NaN());
        aroondown_line->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Add NaNs for the minimum period (need period-1 values before calculation)
    // Python AroonUpDown starts producing values after period values are available
    for (int i = start_idx; i < start_idx + params.period - 1; ++i) {
        aroonup_line->append(std::numeric_limits<double>::quiet_NaN());
        aroondown_line->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Calculate Aroon values for each position where we have enough data
              
    for (int i = start_idx + params.period - 1; i < data_size; ++i) {
        double highest = std::numeric_limits<double>::lowest();
        double lowest = std::numeric_limits<double>::max();
        int highest_periods_ago = params.period; // Initialize to maximum distance
        int lowest_periods_ago = params.period;  // Initialize to maximum distance
        
        
        // Look back 'period' bars to find highest and lowest
        for (int lookback = 0; lookback < params.period; ++lookback) {
            int data_index = i - lookback;
            if (data_index < start_idx) break;
            
            if (data_index < data_size) {
                double high_val = high_array[data_index];
                double low_val = low_array[data_index];
                
                
                if (!std::isnan(high_val) && high_val >= highest) {
                    highest = high_val;
                    highest_periods_ago = lookback;
                }
                
                if (!std::isnan(low_val) && low_val <= lowest) {
                    lowest = low_val;
                    lowest_periods_ago = lookback;
                }
            }
        }
        
        // Calculate Aroon values using Python formula:
        // AroonUp = ((period - periods_since_highest) / period) * 100
        // AroonDown = ((period - periods_since_lowest) / period) * 100
        double aroon_up = 100.0 * (params.period - highest_periods_ago) / params.period;
        double aroon_down = 100.0 * (params.period - lowest_periods_ago) / params.period;
        
        
        aroonup_line->append(aroon_up);
        aroondown_line->append(aroon_down);
    }
    
    // Set buffer indices to the end
    aroonup_line->set_idx(aroonup_line->size() - 1);
    aroondown_line->set_idx(aroondown_line->size() - 1);
}

void AroonUpDown::calculate_lines() {
    if (datas.empty()) {
        return;
    }
    
    // Handle single OHLCV data source or separate high/low data sources
    std::shared_ptr<LineSingle> high_line;
    std::shared_ptr<LineSingle> low_line;
    
    if (datas.size() == 1) {
        // Single OHLCV data source - extract high and low lines
        auto data_source = datas[0];
        if (!data_source || !data_source->lines || data_source->lines->size() < 5) {
            return;
        }
        
        // For OHLCV: High is line 2, Low is line 3
        high_line = data_source->lines->getline(2);
        low_line = data_source->lines->getline(3);
    } else if (datas.size() >= 2) {
        // Separate high and low data sources
        auto high_data = datas[0];
        auto low_data = datas[1];
        
        if (!high_data || !high_data->lines || high_data->lines->size() == 0 ||
            !low_data || !low_data->lines || low_data->lines->size() == 0) {
            return;
        }
        
        high_line = high_data->lines->getline(0);
        low_line = low_data->lines->getline(0);
    } else {
        return;
    }
    
    if (!high_line || !low_line) {
        return;
    }
    
    auto aroonup_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    auto aroondown_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(1));
    if (!aroonup_line || !aroondown_line) {
        return;
    }
    
    // Get the data buffers
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    if (!high_buffer || !low_buffer) {
        return;
    }
    
    // Clear and recalculate all values
    aroonup_line->reset();
    aroondown_line->reset();
    
    const auto& high_array = high_buffer->array();
    const auto& low_array = low_buffer->array();
    int data_size = static_cast<int>(high_array.size());
    
    // Skip initial NaN if present (LineBuffer starts with NaN at index 0)
    int start_idx = 0;
    if (data_size > 0 && std::isnan(high_array[0])) {
        start_idx = 1;
        // Add initial NaN to match data structure
        aroonup_line->append(std::numeric_limits<double>::quiet_NaN());
        aroondown_line->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Add NaNs for the minimum period (need period-1 values before calculation)
    // Python AroonUpDown starts producing values after period values are available
    for (int i = start_idx; i < start_idx + params.period - 1; ++i) {
        aroonup_line->append(std::numeric_limits<double>::quiet_NaN());
        aroondown_line->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Calculate Aroon values for each position where we have enough data
              
    for (int i = start_idx + params.period - 1; i < data_size; ++i) {
        double highest = std::numeric_limits<double>::lowest();
        double lowest = std::numeric_limits<double>::max();
        int highest_periods_ago = params.period; // Initialize to maximum distance
        int lowest_periods_ago = params.period;  // Initialize to maximum distance
        
        
        // Look back 'period' bars to find highest and lowest
        for (int lookback = 0; lookback < params.period; ++lookback) {
            int data_index = i - lookback;
            if (data_index < start_idx) break;
            
            if (data_index < data_size) {
                double high_val = high_array[data_index];
                double low_val = low_array[data_index];
                
                
                if (!std::isnan(high_val) && high_val >= highest) {
                    highest = high_val;
                    highest_periods_ago = lookback;
                }
                
                if (!std::isnan(low_val) && low_val <= lowest) {
                    lowest = low_val;
                    lowest_periods_ago = lookback;
                }
            }
        }
        
        // Calculate Aroon values using Python formula:
        // AroonUp = ((period - periods_since_highest) / period) * 100
        // AroonDown = ((period - periods_since_lowest) / period) * 100
        double aroon_up = 100.0 * (params.period - highest_periods_ago) / params.period;
        double aroon_down = 100.0 * (params.period - lowest_periods_ago) / params.period;
        
        
        aroonup_line->append(aroon_up);
        aroondown_line->append(aroon_down);
    }
    
    // Set buffer indices to the end
    aroonup_line->set_idx(aroonup_line->size() - 1);
    aroondown_line->set_idx(aroondown_line->size() - 1);
}

// AroonOscillator implementation - standalone simple version
AroonOscillator::AroonOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period + 1);
}

AroonOscillator::AroonOscillator(std::shared_ptr<LineSeries> data_source) : Indicator() {
    params.period = 14;  // Default period
    setup_lines();
    _minperiod(params.period + 1);
    
    // For full OHLCV data source, we'll extract high and low during calculation
    this->data = data_source;
    this->datas.push_back(data_source);
}

AroonOscillator::AroonOscillator(std::shared_ptr<LineSeries> data_source, int period) : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    // For full OHLCV data source, we'll extract high and low during calculation
    this->data = data_source;
    this->datas.push_back(data_source);
}

AroonOscillator::AroonOscillator(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low, int period) : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    // Store high and low as separate data sources
    if (high) {
        this->datas.push_back(high);
    }
    if (low) {
        this->datas.push_back(low);
    }
    
    // Set primary data to high line for compatibility
    this->data = high;
}

// Removed undefined constructor - not in header file

void AroonOscillator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void AroonOscillator::calculate_lines() {
    std::shared_ptr<backtrader::LineSingle> high_line;
    std::shared_ptr<backtrader::LineSingle> low_line;
    
    if (datas.size() >= 2) {
        // Two separate data sources for high and low
        auto high_data = datas[0];
        auto low_data = datas[1];
        
        if (!high_data || !high_data->lines || high_data->lines->size() == 0 ||
            !low_data || !low_data->lines || low_data->lines->size() == 0) {
            return;
        }
        
        high_line = high_data->lines->getline(0);
        low_line = low_data->lines->getline(0);
    } else if (datas.size() == 1) {
        // Single OHLCV data source
        auto data = datas[0];
        if (!data || !data->lines || data->lines->size() < 5) {
            return;
        }
        
        // High is line 2, Low is line 3 in SimpleTestDataSeries
        high_line = data->lines->getline(2);
        low_line = data->lines->getline(3);
    } else {
        return;
    }
    
    if (!high_line || !low_line) return;
    
    auto osc_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!osc_line) return;
    
    // Clear and recalculate all values
    osc_line->reset();
    
    // Get the actual data arrays
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    if (!high_buffer || !low_buffer) return;
    
    const auto& high_array = high_buffer->array();
    const auto& low_array = low_buffer->array();
    int data_size = static_cast<int>(high_array.size());
    
    // Debug first few values
    bool debug = false;
    
    // Debug buffer state
    if (debug) {
        std::cout << "AroonOsc calculate_lines: array size=" << data_size << std::endl;
        std::cout << "First few high values: ";
        for (int i = 0; i < std::min(6, data_size); ++i) {
            std::cout << high_array[i] << " ";
        }
        std::cout << std::endl;
    }
    
    // Skip the first NaN value if it exists - LineBuffer starts with a NaN
    int start_idx = 0;
    if (data_size > 0 && std::isnan(high_array[0])) {
        start_idx = 1;
    }
    
    for (int i = start_idx; i < static_cast<int>(high_array.size()); ++i) {
        double aroon_osc;
        
        if (debug && i <= 5) {
            std::cout << "AroonOsc loop iteration i=" << i << ", data_size=" << data_size << ", period=" << params.period << std::endl;
        }
        
        // We need period+1 values to calculate (counting from start_idx)
        if (i - start_idx < params.period) {
            // Not enough data yet
            aroon_osc = std::numeric_limits<double>::quiet_NaN();
            if (debug && i <= 5) {
                std::cout << "  Not enough data (i-start_idx=" << (i-start_idx) << " < " << params.period << "), appending NaN" << std::endl;
            }
        } else {
            // Find highest and lowest in the period + 1 bars (looking back period+1 bars)
            double highest = std::numeric_limits<double>::lowest();
            double lowest = std::numeric_limits<double>::max();
            int highest_idx = 0;
            int lowest_idx = 0;
            
            // Python's FindFirstIndexHighest finds the FIRST (earliest) occurrence of the max
            // It returns the index counting backwards from current (0=current, 1=previous, etc)
            // We need to match this behavior
            
            // Check period + 1 bars back from current position
            for (int j = 0; j <= params.period; ++j) {
                int data_idx = i - j;
                if (data_idx < 0) break;
                
                // Access data directly from the arrays we already have
                if (data_idx >= static_cast<int>(high_array.size()) || 
                    data_idx >= static_cast<int>(low_array.size())) {
                    break;
                }
                
                double high_val = high_array[data_idx];
                double low_val = low_array[data_idx];
                
                // Skip NaN values
                if (std::isnan(high_val) || std::isnan(low_val)) {
                    continue;
                }
                
                // For highest: we want the FIRST (earliest) occurrence
                // >= ensures we keep the first occurrence when values are equal
                if (high_val >= highest) {
                    highest = high_val;
                    highest_idx = j;
                }
                
                // For lowest: we want the FIRST (earliest) occurrence
                // <= ensures we keep the first occurrence when values are equal
                if (low_val <= lowest) {
                    lowest = low_val;
                    lowest_idx = j;
                }
            }
            
            // Calculate Aroon values - consistent with Python formula
            // Formula: 100 * (period - distance_to_extreme) / period
            double aroon_up = 100.0 * (params.period - highest_idx) / params.period;
            double aroon_down = 100.0 * (params.period - lowest_idx) / params.period;
            
            // Aroon Oscillator = AroonUp - AroonDown
            aroon_osc = aroon_up - aroon_down;
            
            // Debug output for the first valid value and key test positions
            if (debug && (i == params.period || i == 120 || i == 134)) {
                std::cout << "AroonOsc Debug at position " << i << ":\n";
                std::cout << "  highest_idx=" << highest_idx << ", lowest_idx=" << lowest_idx << "\n";
                std::cout << "  highest=" << highest << ", lowest=" << lowest << "\n";
                std::cout << "  aroon_up=" << aroon_up << ", aroon_down=" << aroon_down << "\n";
                std::cout << "  aroon_osc=" << aroon_osc << "\n";
            }
        }
        
        if (debug) {
            std::cout << "  Position " << i << ": Appending aroon_osc=" << aroon_osc << " to buffer" << std::endl;
        }
        osc_line->append(aroon_osc);
    }
    
    if (debug) {
        std::cout << "AroonOsc calculate_lines finished. Buffer size: " << osc_line->size() << std::endl;
        std::cout << "Buffer contents: ";
        const auto& osc_array = osc_line->array();
        for (size_t i = 0; i < osc_array.size(); ++i) {
            std::cout << osc_array[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "Buffer _idx=" << osc_line->get_idx() << std::endl;
    }
    
    // Sync the buffer index to point to the last valid value
    // Find the last non-NaN value
    const auto& osc_array = osc_line->array();
    for (int i = static_cast<int>(osc_array.size()) - 1; i >= 0; --i) {
        if (!std::isnan(osc_array[i])) {
            osc_line->set_idx(i);
            if (debug) {
                std::cout << "Set buffer _idx to " << i << " (last valid value)" << std::endl;
            }
            break;
        }
    }
}

void AroonOscillator::calculate() {
    calculate_lines();
}

void AroonOscillator::calculate_lines_at_position(int position) {
    // Simple position-based calculation for compatibility
    calculate_lines();
}

double AroonOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto osc_line = lines->getline(aroonosc);
    if (!osc_line || osc_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Handle ago parameter correctly for Python semantics
    // ago=0 means current (most recent), negative ago means historical
    if (ago > 0) {
        // Positive ago doesn't make sense for historical data
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Convert Python ago convention to C++ LineBuffer indexing
    // Python: ago=-240 means 240 positions back from current
    // C++ LineBuffer: need to use the buffer's array directly
    // For ago=-240: buffer[idx + ago] = buffer[255 + (-240)] = buffer[15]
    
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(osc_line);
    if (!buffer) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    const auto& array = buffer->array();
    int target_index = buffer->get_idx() + ago;
    
    
    if (target_index < 0 || target_index >= static_cast<int>(array.size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return array[target_index];
}

int AroonOscillator::getMinPeriod() const {
    return params.period + 1;
}

size_t AroonOscillator::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto osc_line = lines->getline(aroonosc);
    if (!osc_line) {
        return 0;
    }
    return osc_line->size();
}

// AroonUpDownOscillator implementation
AroonUpDownOscillator::AroonUpDownOscillator() : AroonBase(true, true) {
    setup_lines();
}

void AroonUpDownOscillator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>()); // AroonUp
        lines->add_line(std::make_shared<LineBuffer>()); // AroonDown
        lines->add_line(std::make_shared<LineBuffer>()); // AroonOsc
        lines->add_alias("aroonup", 0);
        lines->add_alias("aroondown", 1);
        lines->add_alias("aroonosc", 2);
    }
}

void AroonUpDownOscillator::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto aroonup_line = lines->getline(aroonup);
    auto aroondown_line = lines->getline(aroondown);
    auto aroonosc_line = lines->getline(aroonosc);
    
    if (!aroonup_line || !aroondown_line || !aroonosc_line) return;
    
    // Find index of highest high and lowest low in period + 1 bars
    int highest_idx = find_highest_index(params.period + 1);
    int lowest_idx = find_lowest_index(params.period + 1);
    
    // Calculate AroonUp and AroonDown
    up_value_ = (100.0 / params.period) * (params.period - highest_idx);
    down_value_ = (100.0 / params.period) * (params.period - lowest_idx);
    
    aroonup_line->set(0, up_value_);
    aroondown_line->set(0, down_value_);
    aroonosc_line->set(0, up_value_ - down_value_);
}

double AroonUpDownOscillator::get(int ago) const {
    if (!lines || lines->size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto aroonosc_line = lines->getline(aroonosc);
    if (!aroonosc_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*aroonosc_line)[ago];
}

int AroonUpDownOscillator::getMinPeriod() const {
    return params.period + 1;
}

} // namespace backtrader