#include "indicators/smaosc.h"
#include "linebuffer.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace backtrader {
namespace indicators {

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator() 
    : Indicator(), data_source_(nullptr), current_index_(0),
      sum_(0.0), first_run_(true) {
    printf("SMAOsc default constructor: params.period = %d\n", params.period);
    setup_lines();
    _minperiod(params.period);
}

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0),
      sum_(0.0), first_run_(true) {
    printf("SMAOsc LineSeries constructor: params.period = %d\n", params.period);
    setup_lines();
    _minperiod(params.period);
    
    // Add data source to datas vector for calculate()
    if (data_source) {
        this->datas.push_back(data_source);
    }
}

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0),
      sum_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Add data source to datas vector for calculate()
    if (data_source) {
        this->datas.push_back(data_source);
    }
}

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator(std::shared_ptr<DataSeries> data_source)
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0),
      sum_(0.0), first_run_(true) {
    printf("SMAOsc DataSeries constructor: params.period = %d\n", params.period);
    setup_lines();
    _minperiod(params.period);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

SimpleMovingAverageOscillator::SimpleMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0),
      sum_(0.0), first_run_(true) {
    params.period = period;
    printf("SMAOsc DataSeries constructor (with period): params.period = %d\n", params.period);
    setup_lines();
    _minperiod(params.period);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

void SimpleMovingAverageOscillator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double SimpleMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        printf("SMAOsc get(%d): lines is null or empty\n", ago);
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto smaosc_line = lines->getline(smaosc);
    if (!smaosc_line) {
        printf("SMAOsc get(%d): smaosc_line is null\n", ago);
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double value = (*smaosc_line)[ago];
    printf("SMAOsc get(%d): returning %f\n", ago, value);
    return value;
}

void SimpleMovingAverageOscillator::calculate() {
    // Batch processing mode: calculate all values at once (matching EMAOsc pattern)
    printf("SMAOsc calculate() called\n");
    if (!lines || lines->size() == 0) {
        setup_lines();
    }
    
    auto smaosc_line = lines->getline(smaosc);
    if (!smaosc_line) {
        printf("SMAOsc calculate: smaosc_line is null\n");
        return;
    }
    
    // Get data source (matching EMAOsc pattern)
    std::shared_ptr<LineSingle> data_line;
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        // Use provided data source
        data_line = data_source_->lines->getline(0);
        printf("SMAOsc calculate: using data_source_ first line\n");
    } else if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 4) {
        // Use close price (line 4) for DataSeries - correct order
        data_line = datas[0]->lines->getline(4);
        printf("SMAOsc calculate: using datas[0] close line\n");
    } else if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        // Use first line for LineSeries
        data_line = datas[0]->lines->getline(0);
        printf("SMAOsc calculate: using datas[0] first line\n");
    }
    
    if (!data_line) {
        printf("SMAOsc calculate: data_line is null\n");
        return;
    }
    
    // Get the LineBuffer for direct array access
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) {
        printf("SMAOsc calculate: data_line is not a LineBuffer\n");
        return;
    }
    
    // For LineBuffer, when _idx is -1, size() returns 0 but array has data
    // Get actual data size from the parent data source
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        data_size = datas[0]->size();  // DataSeries/LineSeries size() returns actual data count
    } else if (data_source_) {
        data_size = data_source_->size();
    }
    
    if (data_size == 0) {
        printf("SMAOsc calculate: no data available\n");
        return;
    }
    
    printf("SMAOsc calculate: data_size = %zu\n", data_size);
    
    auto smaosc_buffer = std::dynamic_pointer_cast<LineBuffer>(smaosc_line);
    if (!smaosc_buffer) {
        return;
    }
    
    // Reset calculation state
    price_buffer_.clear();
    sum_ = 0.0;
    
    // Calculate all values
    for (size_t i = 0; i < data_size; ++i) {
        double price = data_buffer->array()[i];
        
        // Add to price buffer
        price_buffer_.push_back(price);
        sum_ += price;
        if (price_buffer_.size() > static_cast<size_t>(params.period)) {
            sum_ -= price_buffer_.front();
            price_buffer_.pop_front();
        }
        
        // Calculate oscillator when we have enough data
        double oscillator = std::numeric_limits<double>::quiet_NaN();
        if (price_buffer_.size() == static_cast<size_t>(params.period)) {
            double sma = sum_ / params.period;
            oscillator = price - sma;  // Price - SMA
        }
        
        // Append to buffer (critical: use append, not set)
        smaosc_buffer->append(oscillator);
    }
    
    // Set the LineBuffer index to the last position
    smaosc_buffer->set_idx(smaosc_buffer->size() - 1);
}

void SimpleMovingAverageOscillator::next() {
    calculate();
}

void SimpleMovingAverageOscillator::once(int start, int end) {
    // Reset state
    price_buffer_.clear();
    sum_ = 0.0;
    first_run_ = true;
    
    for (int i = start; i < end; ++i) {
        current_index_ = i;
        calculate();
    }
}

size_t SimpleMovingAverageOscillator::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto smaosc_line = lines->getline(smaosc);
    return smaosc_line ? smaosc_line->size() : 0;
}

int SimpleMovingAverageOscillator::getMinPeriod() const {
    // Debug output
    printf("SimpleMovingAverageOscillator getMinPeriod: params.period = %d\n", params.period);
    return params.period;
}

} // namespace indicators
} // namespace backtrader