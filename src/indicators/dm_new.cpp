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

namespace backtrader {
namespace indicators {

DirectionalMovement::DirectionalMovement() : Indicator() {
    setup_lines();
    _minperiod(params.period * 2 - 1); // Simplified minimum period
}

DirectionalMovement::DirectionalMovement(std::shared_ptr<DataSeries> data_source)
    : Indicator() {
    setup_lines();
    _minperiod(params.period * 2 - 1);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

DirectionalMovement::DirectionalMovement(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator() {
    std::cout << "DM: Constructor called with period=" << period << std::endl;
    params.period = period;
    setup_lines();
    _minperiod(params.period * 2 - 1);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

void DirectionalMovement::setup_lines() {
    // Create 4 lines: plusDI, minusDI, dx, adx
    if (!lines) {
        lines = std::make_shared<Lines>();
    }
    
    while (lines->size() < 4) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    
    // Add aliases
    lines->add_alias("plusDI", plusDI);
    lines->add_alias("minusDI", minusDI);
    lines->add_alias("dx", dx);
    lines->add_alias("adx", adx);
    
    // Connect LineCollection to IndicatorBase lines_ vector for size() method
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
}

void DirectionalMovement::calculate() {
    std::cout << "DM: Starting calculate_dm_values, data size=" << (data ? data->lines->size() : 0) << std::endl;
    
    if (!data || !data->lines || data->lines->size() < 5) {
        std::cout << "DM: Invalid data source" << std::endl;
        return;
    }
    
    // Get data lines: high=2, low=3, close=4
    auto high_line = data->lines->getline(2);
    auto low_line = data->lines->getline(3);
    
    if (!high_line || !low_line) {
        std::cout << "DM: Missing high/low lines" << std::endl;
        return;
    }
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    
    if (!high_buffer || !low_buffer) {
        std::cout << "DM: Cannot cast to LineBuffer" << std::endl;
        return;
    }
    
    const auto& high_array = high_buffer->array();
    const auto& low_array = low_buffer->array();
    
    size_t data_size = std::min(high_array.size(), low_array.size());
    if (data_size < 2) {
        std::cout << "DM: Insufficient data size: " << data_size << std::endl;
        return;
    }
    
    // Get output line buffers
    auto plusDI_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(plusDI));
    auto minusDI_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(minusDI));
    auto dx_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(dx));
    auto adx_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(adx));
    
    if (!plusDI_line || !minusDI_line || !dx_line || !adx_line) {
        std::cout << "DM: Output lines not available" << std::endl;
        return;
    }
    
    // Reset all output lines
    plusDI_line->reset();
    minusDI_line->reset();
    dx_line->reset();
    adx_line->reset();
    
    // Create ATR indicator
    auto atr = std::make_shared<ATR>(data, params.period);
    atr->calculate();
    
    auto atr_buffer = std::dynamic_pointer_cast<LineBuffer>(atr->lines->getline(0));
    if (!atr_buffer) {
        std::cout << "DM: ATR calculation failed" << std::endl;
        return;
    }
    
    const auto& atr_array = atr_buffer->array();
    
    // Calculate directional movements and DI values
    std::vector<double> plus_dm_values;
    std::vector<double> minus_dm_values;
    
    plus_dm_values.reserve(data_size);
    minus_dm_values.reserve(data_size);
    
    // First value is NaN
    plus_dm_values.push_back(std::numeric_limits<double>::quiet_NaN());
    minus_dm_values.push_back(std::numeric_limits<double>::quiet_NaN());
    
    // Calculate directional movements
    for (size_t i = 1; i < data_size; ++i) {
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
    
    // Create LineSeries for SMMA calculations
    auto plus_dm_series = std::make_shared<LineSeries>();
    plus_dm_series->lines->add_line(std::make_shared<LineBuffer>());
    auto plus_dm_buffer_out = std::dynamic_pointer_cast<LineBuffer>(plus_dm_series->lines->getline(0));
    
    auto minus_dm_series = std::make_shared<LineSeries>();
    minus_dm_series->lines->add_line(std::make_shared<LineBuffer>());
    auto minus_dm_buffer_out = std::dynamic_pointer_cast<LineBuffer>(minus_dm_series->lines->getline(0));
    
    if (plus_dm_buffer_out && minus_dm_buffer_out) {
        plus_dm_buffer_out->reset();
        minus_dm_buffer_out->reset();
        
        for (const auto& val : plus_dm_values) {
            plus_dm_buffer_out->append(val);
        }
        for (const auto& val : minus_dm_values) {
            minus_dm_buffer_out->append(val);
        }
    }
    
    // Calculate smoothed moving averages
    auto plus_dm_smma = std::make_shared<SMMA>(plus_dm_series, params.period);
    auto minus_dm_smma = std::make_shared<SMMA>(minus_dm_series, params.period);
    
    plus_dm_smma->calculate();
    minus_dm_smma->calculate();
    
    auto plus_dm_avg_buffer = std::dynamic_pointer_cast<LineBuffer>(plus_dm_smma->lines->getline(0));
    auto minus_dm_avg_buffer = std::dynamic_pointer_cast<LineBuffer>(minus_dm_smma->lines->getline(0));
    
    if (!plus_dm_avg_buffer || !minus_dm_avg_buffer) {
        std::cout << "DM: SMMA calculation failed" << std::endl;
        return;
    }
    
    const auto& plus_dm_avg_array = plus_dm_avg_buffer->array();
    const auto& minus_dm_avg_array = minus_dm_avg_buffer->array();
    
    // Calculate DI values and DX
    std::vector<double> dx_values;
    dx_values.reserve(data_size);
    
    for (size_t i = 0; i < data_size; ++i) {
        double plus_di = std::numeric_limits<double>::quiet_NaN();
        double minus_di = std::numeric_limits<double>::quiet_NaN();
        double dx_val = std::numeric_limits<double>::quiet_NaN();
        
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
        dx_values.push_back(dx_val);
    }
    
    // Create DX series for ADX calculation
    auto dx_series = std::make_shared<LineSeries>();
    dx_series->lines->add_line(std::make_shared<LineBuffer>());
    auto dx_buffer_out = std::dynamic_pointer_cast<LineBuffer>(dx_series->lines->getline(0));
    
    if (dx_buffer_out) {
        dx_buffer_out->reset();
        for (const auto& val : dx_values) {
            dx_buffer_out->append(val);
            dx_line->append(val);
        }
    }
    
    // Calculate ADX
    auto adx_smma = std::make_shared<SMMA>(dx_series, params.period);
    adx_smma->calculate();
    
    auto adx_buffer = std::dynamic_pointer_cast<LineBuffer>(adx_smma->lines->getline(0));
    if (adx_buffer) {
        const auto& adx_array = adx_buffer->array();
        for (const auto& val : adx_array) {
            adx_line->append(val);
        }
    }
    
    // Set line indices
    if (plusDI_line->size() > 0) plusDI_line->set_idx(plusDI_line->size() - 1);
    if (minusDI_line->size() > 0) minusDI_line->set_idx(minusDI_line->size() - 1);
    if (dx_line->size() > 0) dx_line->set_idx(dx_line->size() - 1);
    if (adx_line->size() > 0) adx_line->set_idx(adx_line->size() - 1);
    
    std::cout << "DM: Finished calculate_dm_values, plusDI size=" << plusDI_line->size() 
              << ", first few values: ";
    for (int i = 0; i < std::min(5, static_cast<int>(plusDI_line->size())); ++i) {
        std::cout << (*plusDI_line)[i] << " ";
    }
    std::cout << std::endl;
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
    return (*line)[ago];
}

double DirectionalMovement::getDIMinus(int ago) const {
    if (!lines) return std::numeric_limits<double>::quiet_NaN();
    auto line = lines->getline(minusDI);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    return (*line)[ago];
}

double DirectionalMovement::getDX(int ago) const {
    if (!lines) return std::numeric_limits<double>::quiet_NaN();
    auto line = lines->getline(dx);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    return (*line)[ago];
}

double DirectionalMovement::getADX(int ago) const {
    if (!lines) return std::numeric_limits<double>::quiet_NaN();
    auto line = lines->getline(adx);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
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
    return params.period * 2 - 1;
}

} // namespace indicators
} // namespace backtrader