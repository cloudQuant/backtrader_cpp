#include "indicators/dm.h"
#include "indicators/atr.h"
#include "indicators/smma.h"
#include "../include/indicator_utils.h"
#include "linebuffer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
#include <typeinfo>

namespace backtrader {
namespace indicators {

DirectionalMovement::DirectionalMovement() : Indicator() {
    setup_lines();
    // Python DM minimum period is 3*period = 42 for period=14
    _minperiod(params.period * 3);
}

DirectionalMovement::DirectionalMovement(std::shared_ptr<DataSeries> data_source)
    : Indicator() {
    setup_lines();
    // Python DM minimum period is 3*period = 42 for period=14
    _minperiod(params.period * 3);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

DirectionalMovement::DirectionalMovement(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator() {
    params.period = period;
    setup_lines();
    // Python DM minimum period is 3*period = 42 for period=14
    _minperiod(params.period * 3);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

void DirectionalMovement::setup_lines() {
    // Create 4 lines: adx, adxr, plusDI, minusDI (matches Python order)
    if (!lines) {
        lines = std::make_shared<Lines>();
    }
    
    while (lines->size() < 4) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    
    // Add aliases - matches Python order
    lines->add_alias("adx", adx);
    lines->add_alias("adxr", adxr);
    lines->add_alias("plusDI", plusDI);
    lines->add_alias("minusDI", minusDI);
    
    // Connect LineCollection to IndicatorBase lines_ vector for size() method
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
}

void DirectionalMovement::calculate() {
    if (!data || !data->lines) {
        return;
    }
    
    if (data->lines->size() < 5) {
        return;
    }
    
    // Get data lines: datetime=0, open=1, high=2, low=3, close=4
    auto high_line = data->lines->getline(DataSeries::High);   // Should be 2
    auto low_line = data->lines->getline(DataSeries::Low);     // Should be 3
    
    if (!high_line || !low_line) {
        return;
    }
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    
    if (!high_buffer || !low_buffer) {
        return;
    }
    
    const auto& high_array = high_buffer->array();
    const auto& low_array = low_buffer->array();
    
    size_t data_size = std::min(high_array.size(), low_array.size());
    if (data_size < 2) {
        return;
    }
    
    // Get output line buffers - using correct order
    auto adx_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(adx));
    auto adxr_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(adxr));
    auto plusDI_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(plusDI));
    auto minusDI_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(minusDI));
    
    if (!adx_line || !adxr_line || !plusDI_line || !minusDI_line) {
        return;
    }
    
    // Reset all output lines
    adx_line->reset();
    adxr_line->reset();
    plusDI_line->reset();
    minusDI_line->reset();
    
    // Create ATR indicator
    // Ensure we cast to DataSeries to call the correct constructor
    auto data_series = std::dynamic_pointer_cast<DataSeries>(data); 
    if (!data_series) {
        return;
    }
    
    auto atr = std::make_shared<ATR>(data_series, params.period);
    
    // Calculate ATR for all data
    atr->calculate();
    
    // Get ATR buffer
    auto atr_buffer = std::dynamic_pointer_cast<LineBuffer>(atr->lines->getline(0));
    if (!atr_buffer) {
        return;
    }
    
    const auto& atr_array = atr_buffer->array();
    
    // Calculate directional movements and DI values
    std::vector<double> plus_dm_values;
    std::vector<double> minus_dm_values;
    
    // Use ATR buffer size to ensure consistency
    size_t atr_size = atr_buffer->data_size();
    plus_dm_values.reserve(atr_size);
    minus_dm_values.reserve(atr_size);
    
    // First value is NaN
    plus_dm_values.push_back(std::numeric_limits<double>::quiet_NaN());
    minus_dm_values.push_back(std::numeric_limits<double>::quiet_NaN());
    
    
    // Calculate directional movements - ensure we don't exceed array bounds
    size_t target_size = std::min(atr_size, data_size);
    // Make sure we don't go beyond the actual data arrays
    target_size = std::min(target_size, high_array.size());
    target_size = std::min(target_size, low_array.size());
    
    for (size_t i = 1; i < target_size; ++i) {
        double high_curr = high_array[i];
        double high_prev = high_array[i-1];
        double low_curr = low_array[i];
        double low_prev = low_array[i-1];
        
        double up_move = high_curr - high_prev;
        double down_move = low_prev - low_curr;
        
        double plus_dm = 0.0;
        double minus_dm = 0.0;
        
        // +DM = up_move if (up_move > down_move and up_move > 0) else 0
        if (up_move > down_move && up_move > 0.0) {
            plus_dm = up_move;
        }
        
        // -DM = down_move if (down_move > up_move and down_move > 0) else 0  
        if (down_move > up_move && down_move > 0.0) {
            minus_dm = down_move;
        }
        
        plus_dm_values.push_back(plus_dm);
        minus_dm_values.push_back(minus_dm);
    }
    
    
    // If we need more values to match ATR size, add NaN values
    while (plus_dm_values.size() < atr_size) {
        plus_dm_values.push_back(std::numeric_limits<double>::quiet_NaN());
        minus_dm_values.push_back(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Calculate smoothed moving averages manually (bypass SMMA class issues)
    std::vector<double> plus_dm_avg_array(plus_dm_values.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> minus_dm_avg_array(minus_dm_values.size(), std::numeric_limits<double>::quiet_NaN());
    
    // Calculate SMMA using Wilder's smoothing method
    // First value: simple average of first period values
    // Subsequent values: SMMA = (prev_SMMA * (period-1) + new_value) / period
    
    // Calculate SMMA starting from when we have enough values
    // Note: In Python backtrader, SMMA starts at index period-1 (0-based)
    for (size_t i = params.period - 1; i < plus_dm_values.size(); ++i) {
        if (i == params.period - 1) {
            // Initial SMMA: simple average of first period values
            double plus_sum = 0.0, minus_sum = 0.0;
            int plus_count = 0, minus_count = 0;
            
            for (size_t j = 0; j < params.period; ++j) {
                if (!std::isnan(plus_dm_values[j])) {
                    plus_sum += plus_dm_values[j];
                    plus_count++;
                }
                if (!std::isnan(minus_dm_values[j])) {
                    minus_sum += minus_dm_values[j];
                    minus_count++;
                }
            }
            
            // For SMMA, we still divide by period, not by count
            plus_dm_avg_array[i] = plus_sum / params.period;
            minus_dm_avg_array[i] = minus_sum / params.period;
            
        } else {
            // Subsequent SMMA values using Wilder's smoothing
            double prev_plus_smma = plus_dm_avg_array[i-1];
            double prev_minus_smma = minus_dm_avg_array[i-1];
            
            if (!std::isnan(prev_plus_smma) && !std::isnan(prev_minus_smma)) {
                plus_dm_avg_array[i] = (prev_plus_smma * (params.period - 1) + plus_dm_values[i]) / params.period;
                minus_dm_avg_array[i] = (prev_minus_smma * (params.period - 1) + minus_dm_values[i]) / params.period;
            }
        }
    }
    
    // Calculate DI values and DX using manual SMMA results
    std::vector<double> dx_values;
    dx_values.reserve(data_size);
    
    // DI and DX calculation
    for (size_t i = 0; i < data_size; ++i) {
        double plus_di = std::numeric_limits<double>::quiet_NaN();
        double minus_di = std::numeric_limits<double>::quiet_NaN();
        double dx_val = std::numeric_limits<double>::quiet_NaN();
        
        // Check if we have corresponding SMMA and ATR values
        if (i < plus_dm_avg_array.size() && i < minus_dm_avg_array.size() && i < atr_array.size()) {
            double plus_dm_avg = plus_dm_avg_array[i];
            double minus_dm_avg = minus_dm_avg_array[i];
            double atr_val = atr_array[i];
            
            if (!std::isnan(plus_dm_avg) && !std::isnan(minus_dm_avg) && !std::isnan(atr_val) && atr_val > 0.0) {
                plus_di = 100.0 * plus_dm_avg / atr_val;
                minus_di = 100.0 * minus_dm_avg / atr_val;
                
                double di_sum = plus_di + minus_di;
                if (di_sum > 0.0) {
                    dx_val = 100.0 * std::abs(plus_di - minus_di) / di_sum;
                }
            }
        }
        
        plusDI_line->append(plus_di);
        minusDI_line->append(minus_di);
        // Store dx values for ADX calculation
        dx_values.push_back(dx_val);
    }
    
    
    // Calculate ADX manually using SMMA of DX values
    std::vector<double> adx_values(data_size, std::numeric_limits<double>::quiet_NaN());
    
    // Calculate ADX SMMA
    // ADX starts at (period*2 - 1) to match Python behavior
    size_t adx_start = params.period * 2 - 1;
    for (size_t i = adx_start; i < data_size && i < dx_values.size(); ++i) {
        if (i == adx_start) {
            // Initial ADX: simple average of first period DX values
            double dx_sum = 0.0;
            int valid_count = 0;
            
            // Calculate average of period DX values ending at current position
            for (size_t j = i - params.period + 1; j <= i; ++j) {
                if (!std::isnan(dx_values[j])) {
                    dx_sum += dx_values[j];
                    valid_count++;
                }
            }
            
            if (valid_count > 0) {
                // For SMMA, we divide by period even if some values are NaN
                adx_values[i] = dx_sum / params.period;
            }
        } else {
            // Subsequent ADX values using Wilder's smoothing
            double prev_adx = adx_values[i-1];
            
            if (!std::isnan(prev_adx) && !std::isnan(dx_values[i])) {
                adx_values[i] = (prev_adx * (params.period - 1) + dx_values[i]) / params.period;
            }
        }
    }
    
    // Append ADX values to output line
    for (const auto& val : adx_values) {
        adx_line->append(val);
    }
    
    // Calculate ADXR (Average Directional Movement Index Rating)
    // ADXR = (ADX + ADX from period days ago) / 2
    for (size_t i = 0; i < data_size; ++i) {
        double adxr_val = std::numeric_limits<double>::quiet_NaN();
        
        if (i >= params.period && i < adx_values.size()) {
            double current_adx = adx_values[i];
            double past_adx = adx_values[i - params.period];
            
            if (!std::isnan(current_adx) && !std::isnan(past_adx)) {
                adxr_val = (current_adx + past_adx) / 2.0;
            }
        }
        
        adxr_line->append(adxr_val);
    }
    
    // Set line indices
    if (adx_line->size() > 0) adx_line->set_idx(adx_line->size() - 1);
    if (adxr_line->size() > 0) adxr_line->set_idx(adxr_line->size() - 1);
    if (plusDI_line->size() > 0) plusDI_line->set_idx(plusDI_line->size() - 1);
    if (minusDI_line->size() > 0) minusDI_line->set_idx(minusDI_line->size() - 1);
    
}

void DirectionalMovement::next() {
    calculate();
}

void DirectionalMovement::once(int start, int end) {
    calculate();
}

double DirectionalMovement::get(int ago) const {
    // Return ADX as the main line for test framework
    return getADX(ago);
}

double DirectionalMovement::getDIPlus(int ago) const {
    if (!lines) return std::numeric_limits<double>::quiet_NaN();
    auto line = lines->getline(plusDI);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    
    // Use operator[] directly which we know works
    return (*line)[ago];
}

double DirectionalMovement::getDIMinus(int ago) const {
    if (!lines) return std::numeric_limits<double>::quiet_NaN();
    auto line = lines->getline(minusDI);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    
    // Use operator[] directly which we know works
    return (*line)[ago];
}

double DirectionalMovement::getADXR(int ago) const {
    if (!lines) return std::numeric_limits<double>::quiet_NaN();
    auto line = lines->getline(adxr);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    
    // Use operator[] directly which we know works
    return (*line)[ago];
}

double DirectionalMovement::getADX(int ago) const {
    if (!lines) return std::numeric_limits<double>::quiet_NaN();
    auto line = lines->getline(adx);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    
    // Use operator[] directly which we know works
    return (*line)[ago];
}

size_t DirectionalMovement::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto plusDI_line = lines->getline(plusDI);
    return plusDI_line ? plusDI_line->size() : 0;
}

int DirectionalMovement::getMinPeriod() const {
    return params.period * 3;
}

} // namespace indicators
} // namespace backtrader