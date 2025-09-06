#include "indicators/accdecoscillator.h"
#include "indicators/awesomeoscillator.h"
#include "indicators/sma.h"
#include "../include/indicator_utils.h"
#include <limits>
#include <cmath>
#include <iostream>

namespace backtrader {

// AccelerationDecelerationOscillator implementation
AccelerationDecelerationOscillator::AccelerationDecelerationOscillator() : Indicator(), calculate_called(0) {
    setup_lines();
    _minperiod(38);  // AO period (34) + SMA period (5) - 1
    
    // std::cerr << "AccDecOsc default constructor: calculate_called = " << calculate_called << std::endl;
}

AccelerationDecelerationOscillator::AccelerationDecelerationOscillator(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low) : Indicator(), calculate_called(0) {
    // std::cout << "AccDecOsc two-param constructor called" << std::endl;
    setup_lines();
    _minperiod(38);  // AO period (34) + SMA period (5) - 1
    
    // Store the high and low data for later use
    high_data_ = high;
    low_data_ = low;
}

AccelerationDecelerationOscillator::AccelerationDecelerationOscillator(std::shared_ptr<LineSeries> data_source) : Indicator(), calculate_called(0) {
    setup_lines();
    _minperiod(38);  // AO period (34) + SMA period (5) - 1
    
    // This constructor is for test framework compatibility
    data = data_source;
    datas.push_back(data_source);
    
    // std::cerr << "AccDecOsc LineSeries constructor: calculate_called = " << calculate_called << std::endl;
}

AccelerationDecelerationOscillator::AccelerationDecelerationOscillator(std::shared_ptr<DataSeries> data_source) : Indicator(), calculate_called(0) {
    setup_lines();
    _minperiod(38);  // AO period (34) + SMA period (5) - 1
    
    // This constructor is for test framework compatibility
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        data = lineseries;
        datas.push_back(lineseries);
    }
    
    // std::cerr << "AccDecOsc DataSeries constructor: calculate_called = " << calculate_called << std::endl;
}

void AccelerationDecelerationOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("accde", 0);
    }
    
    // Connect LineCollection to IndicatorBase lines_ vector for size() method
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
}

double AccelerationDecelerationOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto accde_line = lines->getline(accde);
    if (!accde_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*accde_line)[ago];
}

int AccelerationDecelerationOscillator::getMinPeriod() const {
    return 38; // AO period (34) + SMA period (5) - 1
}

size_t AccelerationDecelerationOscillator::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto accde_line = lines->getline(accde);
    return accde_line ? accde_line->size() : 0;
}

void AccelerationDecelerationOscillator::calculate() {
    calculate_called = 1;
    
    // Determine data source
    std::shared_ptr<LineSeries> data_source = nullptr;
    size_t data_size = 0;
    
    if (!datas.empty() && datas[0]) {
        data_source = datas[0];
        // std::cout << "Calculate: datas[0]->lines->size() = " << datas[0]->lines->size() << std::endl;
        
        // Get data size from close line (index 3) or first available line
        auto close_line = datas[0]->lines->getline(3);
        if (!close_line) {
            close_line = datas[0]->lines->getline(0);
        }
        if (close_line) {
            data_size = close_line->size();
        }
    } else if (high_data_ && low_data_) {
        // std::cout << "Calculate: Using high_data_ and low_data_" << std::endl;
        
        // Get data size from high_data_
        auto high_line = high_data_->lines->getline(0);
        if (high_line) {
            data_size = high_line->size();
        }
    } else {
        // std::cout << "Calculate: No valid data source available" << std::endl;
        return;
    }
    
    if (data_size == 0) {
        // std::cout << "Calculate: No data available, data_size = 0" << std::endl;
        return;
    }
    
    // std::cerr << "AccDecOsc::calculate - Calling once(0, " << data_size << ")" << std::endl;
    once(0, static_cast<int>(data_size));
    
    // Debug output
    auto accde_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(accde));
    if (accde_line) {
        // std::cerr << "AccDecOsc::calculate - After once, idx=" << accde_line->get_idx() 
        //           << ", buflen=" << accde_line->buflen() 
        //           << ", size=" << accde_line->size() << std::endl;
    }
}

void AccelerationDecelerationOscillator::next() {
    // Simple implementation for single-step calculation
    calculate();
}

void AccelerationDecelerationOscillator::once(int start, int end) {
    // std::cerr << "AccDecOsc::once called with start=" << start << ", end=" << end << std::endl;
    
    // Get the output line
    auto accde_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(accde));
    if (!accde_line) {
        std::cerr << "AccDecOsc::once - Failed to get accde line" << std::endl;
        return;
    }
    
    // Clear the output line
    accde_line->reset();
    
    // Create data source for AwesomeOscillator
    std::shared_ptr<LineSeries> ao_data_source = nullptr;
    
    if (!datas.empty() && datas[0]) {
        // Use the existing data source
        ao_data_source = datas[0];
    } else if (high_data_ && low_data_) {
        // std::cerr << "AccDecOsc::once - Using high_data_ and low_data_" << std::endl;
        
        // Create a combined data source with HL2 data
        ao_data_source = std::make_shared<LineSeries>();
        ao_data_source->lines->add_line(std::make_shared<LineBuffer>());
        
        auto high_line = high_data_->lines->getline(0);
        auto low_line = low_data_->lines->getline(0);
        
        if (!high_line || !low_line) {
            // std::cerr << "AccDecOsc::once - high_line or low_line is null" << std::endl;
            return;
        }
        
        auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
        auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
        
        if (!high_buffer || !low_buffer) {
            // std::cerr << "AccDecOsc::once - Failed to cast to LineBuffer" << std::endl;
            return;
        }
        
        const auto& high_array = high_buffer->array();
        const auto& low_array = low_buffer->array();
        
        size_t min_size = std::min(high_array.size(), low_array.size());
        if (min_size == 0) {
            // std::cerr << "AccDecOsc::once - No data in high/low arrays" << std::endl;
            return;
        }
        
        // Create HL2 data
        auto hl2_buffer = std::dynamic_pointer_cast<LineBuffer>(ao_data_source->lines->getline(0));
        if (hl2_buffer) {
            hl2_buffer->reset();
            for (size_t i = 0; i < min_size; ++i) {
                double hl2 = (high_array[i] + low_array[i]) / 2.0;
                hl2_buffer->append(hl2);
            }
        }
    } else {
        // std::cerr << "AccDecOsc::once - No valid data source found" << std::endl;
        return;
    }
    
    // Create AwesomeOscillator
    std::shared_ptr<backtrader::AwesomeOscillator> ao;
    
    if (!datas.empty() && datas[0]) {
        // When using a full data source, pass it directly
        ao = std::make_shared<backtrader::AwesomeOscillator>(ao_data_source);
    } else if (high_data_ && low_data_) {
        // When we have separate high/low data, use the appropriate constructor
        ao = std::make_shared<backtrader::AwesomeOscillator>(high_data_, low_data_);
    } else {
        // std::cerr << "AccDecOsc::once - Cannot create AwesomeOscillator, no valid data" << std::endl;
        return;
    }
    
    ao->calculate();
    
    // Get AO results
    auto ao_line = ao->lines->getline(0);
    if (!ao_line) {
        // std::cerr << "AccDelOsc::once - Failed to get AO line" << std::endl;
        return;
    }
    
    auto ao_buffer = std::dynamic_pointer_cast<LineBuffer>(ao_line);
    if (!ao_buffer) {
        // std::cerr << "AccDecOsc::once - Failed to cast AO line to buffer" << std::endl;
        return;
    }
    
    const auto& ao_array = ao_buffer->array();
    if (ao_array.empty()) {
        // std::cerr << "AccDecOsc::once - AO array is empty" << std::endl;
        return;
    }
    
    // Create SMA of AO
    auto ao_line_series = std::make_shared<LineSeries>();
    ao_line_series->lines->add_line(std::make_shared<LineBuffer>());
    auto ao_sma_buffer = std::dynamic_pointer_cast<LineBuffer>(ao_line_series->lines->getline(0));
    
    if (ao_sma_buffer) {
        ao_sma_buffer->reset();
        for (const auto& val : ao_array) {
            ao_sma_buffer->append(val);
        }
    }
    
    auto sma = std::make_shared<backtrader::indicators::SMA>(ao_line_series, 5);
    sma->calculate();
    
    // Get SMA results
    auto sma_line = sma->lines->getline(0);
    if (!sma_line) {
        // std::cerr << "AccDecOsc::once - Failed to get SMA line" << std::endl;
        return;
    }
    
    auto sma_buffer = std::dynamic_pointer_cast<LineBuffer>(sma_line);
    if (!sma_buffer) {
        // std::cerr << "AccDecOsc::once - Failed to cast SMA line to buffer" << std::endl;
        return;
    }
    
    const auto& sma_array = sma_buffer->array();
    
    // Calculate AccDecOsc = AO - SMA(AO, 5)
    size_t result_size = std::min(ao_array.size(), sma_array.size());
    
    for (size_t i = 0; i < result_size; ++i) {
        double ao_val = ao_array[i];
        double sma_val = sma_array[i];
        
        if (std::isnan(ao_val) || std::isnan(sma_val)) {
            accde_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            double accde_val = ao_val - sma_val;
            accde_line->append(accde_val);
        }
    }
    
    // Set the line index
    if (accde_line->size() > 0) {
        accde_line->set_idx(accde_line->size() - 1);
    }
    
    // std::cerr << "AccDecOsc::once - After append, idx: " << accde_line->get_idx() 
    //           << ", buflen: " << accde_line->buflen() 
    //           << ", size: " << accde_line->size() 
    //           << ", array size: " << accde_line->array().size() << std::endl;
    
    // Debug: verify some values
    if (accde_line->size() > 0) {
        // std::cerr << "AccDecOsc::once - First few values: ";
        // for (int i = 0; i < std::min(5, static_cast<int>(accde_line->size())); ++i) {
        //     std::cerr << accde_line->get(i) << " ";
        // }
        // std::cerr << std::endl;
    }
}

} // namespace backtrader