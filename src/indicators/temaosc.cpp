#include "indicators/temaosc.h"
#include "indicators/dema.h"  // For TEMA
#include "linebuffer.h"
#include <cmath>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

TripleExponentialMovingAverageOscillator::TripleExponentialMovingAverageOscillator() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Set minperiod: for TEMA with period p, it's (p-1)*3+1
    _minperiod(getMinPeriod());
}

TripleExponentialMovingAverageOscillator::TripleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    // Set minperiod
    _minperiod(getMinPeriod());
    
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
        
        // Create TEMA indicator
        tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, params.period);
    }
}

TripleExponentialMovingAverageOscillator::TripleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Set minperiod
    _minperiod(getMinPeriod());
    
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
        
        // Create TEMA indicator
        tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, params.period);
    }
}

TripleExponentialMovingAverageOscillator::TripleExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source)
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Set minperiod
    _minperiod(getMinPeriod());
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
        this->data_source_ = lineseries;
        
        // Create TEMA indicator
        tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, params.period);
    }
}

TripleExponentialMovingAverageOscillator::TripleExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Set minperiod
    _minperiod(getMinPeriod());
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
        this->data_source_ = lineseries;
        
        // Create TEMA indicator
        tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, params.period);
    }
}

void TripleExponentialMovingAverageOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double TripleExponentialMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return 0.0;
    }
    
    auto temaosc_line = lines->getline(temaosc);
    if (!temaosc_line) {
        return 0.0;
    }
    
    return (*temaosc_line)[ago];
}

int TripleExponentialMovingAverageOscillator::getMinPeriod() const {
    // For TEMA with period p, minperiod is (p-1)*3+1
    // With default period=30: (30-1)*3+1 = 88
    return (params.period - 1) * 3 + 1;
}

void TripleExponentialMovingAverageOscillator::calculate() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get the appropriate line based on the data type
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() >= 5) {
        // This is a DataSeries with OHLC data, use close
        data_line = datas[0]->lines->getline(4); // Close price
    } else if (datas[0]->lines->size() > 0) {
        // This is a simple LineSeries, use the first line
        data_line = datas[0]->lines->getline(0);
    }
    
    if (!data_line || data_line->size() == 0) return;
    
    // Use once() to calculate all values at once
    once(0, data_line->size());
}

void TripleExponentialMovingAverageOscillator::next() {
    calculate();
}

size_t TripleExponentialMovingAverageOscillator::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto temaosc_line = lines->getline(temaosc);
    return temaosc_line ? temaosc_line->size() : 0;
}

void TripleExponentialMovingAverageOscillator::once(int start, int end) {
    if (!tema_ || datas.empty() || !datas[0]->lines) return;
    
    // Get the appropriate line based on the data type
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() >= 5) {
        // This is a DataSeries with OHLC data, use close
        data_line = datas[0]->lines->getline(4); // Close price
    } else if (datas[0]->lines->size() > 0) {
        // This is a simple LineSeries, use the first line
        data_line = datas[0]->lines->getline(0);
    }
    
    if (!data_line) return;
    
    auto temaosc_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(temaosc));
    if (!temaosc_buffer) return;
    
    // Reset the buffer (keeps initial NaN for alignment)
    temaosc_buffer->reset();
    
    // Calculate TEMA first
    tema_->calculate();
    
    // Get TEMA line
    auto tema_line = tema_->lines->getline(0);
    if (!tema_line) return;
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    auto tema_buffer = std::dynamic_pointer_cast<LineBuffer>(tema_line);
    if (!data_buffer || !tema_buffer) return;
    
    const auto& data_array = data_buffer->array();
    const auto& tema_array = tema_buffer->array();
    
    // TEMAOsc = data - TEMA(data)
    // IMPORTANT: Both arrays need to be the same size and properly aligned
    // including the initial NaN value
    size_t max_size = std::min(data_array.size(), tema_array.size());
    
    // Start from index 1 to skip the initial NaN that reset() adds
    // This way we maintain alignment with the data and TEMA buffers
    for (size_t i = 1; i < max_size; ++i) {
        double data_val = data_array[i];
        double tema_val = tema_array[i];
        
        if (std::isnan(data_val) || std::isnan(tema_val)) {
            // Preserve NaN values to maintain alignment
            temaosc_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // TEMAOsc = data - TEMA
            // This creates an oscillator that shows the difference between
            // the raw data and its triple exponentially smoothed version
            double osc_val = data_val - tema_val;
            temaosc_buffer->append(osc_val);
        }
    }
    
    // Set the buffer index to match the data buffer index
    // After reset() and appending (max_size-1) values, _idx should be max_size-1
    int data_idx = data_buffer->get_idx();
    temaosc_buffer->set_idx(data_idx, true);
}

} // namespace indicators
} // namespace backtrader