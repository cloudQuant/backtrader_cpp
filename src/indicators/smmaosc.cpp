#include "indicators/smmaosc.h"
#include <cmath>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

// SmoothedMovingAverageOscillator implementation
SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
    
    // Create SMMA indicator (will be initialized later)
    smma_ = nullptr;
}

SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
    
    // Create SMMA indicator
    smma_ = std::make_shared<SMMA>(data_source, params.period);
    
    // Set up data connection
    this->data = data_source;
    this->datas.push_back(data_source);
}

SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Create SMMA indicator
    smma_ = std::make_shared<SMMA>(data_source, params.period);
    
    // Set up data connection
    this->data = data_source;
    this->datas.push_back(data_source);
}

SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
    
    // Convert DataSeries to LineSeries for SMMA
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        data_source_ = lineseries;
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
    
    // Create SMMA indicator
    smma_ = std::make_shared<SMMA>(lineseries, params.period);
}

SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Convert DataSeries to LineSeries for SMMA
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        data_source_ = lineseries;
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
    
    // Create SMMA indicator
    smma_ = std::make_shared<SMMA>(lineseries, params.period);
}

double SmoothedMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(smmaosc);
    if (!line || line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int SmoothedMovingAverageOscillator::getMinPeriod() const {
    return params.period;
}

size_t SmoothedMovingAverageOscillator::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto smmaosc_line = lines->getline(smmaosc);
    return smmaosc_line ? smmaosc_line->size() : 0;
}

void SmoothedMovingAverageOscillator::calculate() {
    // For batch calculation (like in tests), we need to process all data at once
    int data_size = 0;
    
    std::cerr << "SMMAOsc::calculate() called, data_source_=" << data_source_.get() 
              << ", data=" << data.get() << std::endl;
    
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        auto data_line = data_source_->lines->getline(0);
        if (data_line) {
            auto buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (buffer) {
                // Use data_size() which returns the actual number of data points
                data_size = buffer->data_size();
            } else {
                data_size = data_line->size();
            }
        }
    } else if (data && data->lines && data->lines->size() > 0) {
        auto data_line = data->lines->getline(0);
        if (data_line) {
            auto buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (buffer) {
                // Use data_size() which returns the actual number of data points
                data_size = buffer->data_size();
            } else {
                data_size = data_line->size();
            }
        }
    }
    
    if (data_size > 0) {
        std::cerr << "SMMAOsc::calculate() calling once(0, " << data_size << ")" << std::endl;
        once(0, data_size);
        std::cerr << "SMMAOsc::calculate() after once, buffer size: " << lines->getline(smmaosc)->size() << std::endl;
    }
}

void SmoothedMovingAverageOscillator::prenext() {
    if (smma_) smma_->calculate();
}

void SmoothedMovingAverageOscillator::next() {
    if (!smma_) return;
    
    auto smmaosc_line = lines->getline(smmaosc);
    if (!smmaosc_line) return;
    
    // Calculate SMMA value
    smma_->calculate();
    
    // Get data value and SMMA value
    double data_value = std::numeric_limits<double>::quiet_NaN();
    if (datas.size() > 0 && datas[0] && datas[0]->lines) {
        auto data_line = datas[0]->lines->getline(0);
        if (data_line) {
            data_value = (*data_line)[0];
        }
    }
    
    double smma_value = smma_->get(0);
    
    // Calculate oscillator: data - SMMA(data)
    double oscillator = std::numeric_limits<double>::quiet_NaN();
    if (!std::isnan(data_value) && !std::isnan(smma_value)) {
        oscillator = data_value - smma_value;
    }
    
    smmaosc_line->set(0, oscillator);
}

void SmoothedMovingAverageOscillator::once(int start, int end) {
    if (!smma_) {
        std::cerr << "SMMAOsc::once() - smma_ is null" << std::endl;
        return;
    }
    
    auto smmaosc_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(smmaosc));
    if (!smmaosc_line) {
        std::cerr << "SMMAOsc::once() - smmaosc_line is null" << std::endl;
        return;
    }
    
    std::cerr << "SMMAOsc::once() - before calculate, start=" << start << ", end=" << end << std::endl;
    
    // First, calculate all SMMA values
    smma_->calculate();
    
    // Get the SMMA line buffer
    auto smma_line = smma_->lines->getline(0);
    if (!smma_line) {
        std::cerr << "SMMAOsc::once() - smma_line is null" << std::endl;
        return;
    }
    
    // Get the data line - try to get as LineBuffer
    // For DataSeries, we need the close line (index 4), not the datetime line (index 0)
    std::shared_ptr<LineBuffer> data_buffer;
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        // Check if this is a full DataSeries (with OHLCV) or just a single line
        int line_index = (data_source_->lines->size() > 4) ? 4 : 0;  // 4 = close, 0 = single line
        auto line = data_source_->lines->getline(line_index);
        data_buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    } else if (data && data->lines && data->lines->size() > 0) {
        // Check if this is a full DataSeries (with OHLCV) or just a single line
        int line_index = (data->lines->size() > 4) ? 4 : 0;  // 4 = close, 0 = single line
        auto line = data->lines->getline(line_index);
        data_buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    }
    
    if (!data_buffer) {
        std::cerr << "SMMAOsc::once() - data_buffer is null" << std::endl;
        return;
    }
    
    // Use data_size() for actual data count
    int actual_data_size = data_buffer->data_size();
    
    std::cerr << "SMMAOsc::once() - smma_line size=" << smma_line->size() 
              << ", data_buffer data_size=" << actual_data_size << std::endl;
    
    // Clear and reset the oscillator line only if we're starting from 0
    if (start == 0) {
        smmaosc_line->reset();
    }
    
    // Calculate oscillator values: data - SMMA(data)
    // Access data from the buffer using array indexing
    const auto& data_array = data_buffer->array();
    
    for (int i = start; i < end; ++i) {
        // Access data directly from array
        double data_value = (i < static_cast<int>(data_array.size())) ? data_array[i] : std::numeric_limits<double>::quiet_NaN();
        
        // Calculate the 'ago' value for SMMA access
        int ago = -(actual_data_size - 1 - i);
        double smma_value = smma_->get(ago);
        
        std::cerr << "i=" << i << ", ago=" << ago << ": data=" << data_value << ", smma=" << smma_value << std::endl;
        
        // Calculate oscillator: data - SMMA(data)
        double oscillator = std::numeric_limits<double>::quiet_NaN();
        if (!std::isnan(data_value) && !std::isnan(smma_value)) {
            oscillator = data_value - smma_value;
        }
        
        smmaosc_line->append(oscillator);
    }
}

void SmoothedMovingAverageOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("smmaosc", 0);
    }
}

} // namespace indicators
} // namespace backtrader