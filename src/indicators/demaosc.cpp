#include "indicators/demaosc.h"
#include "linebuffer.h"
#include "../include/indicator_utils.h"
#include <cmath>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

DoubleExponentialMovingAverageOscillator::DoubleExponentialMovingAverageOscillator() 
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema1_(0.0), ema2_(0.0), first_run_(true) {
    // Note: params.period is initialized in the header to 30
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    
    // Set minperiod
    _minperiod(getMinPeriod());
}

DoubleExponentialMovingAverageOscillator::DoubleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0),
      ema1_(0.0), ema2_(0.0), first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    
    // Set minperiod
    _minperiod(getMinPeriod());
    
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
    }
}

DoubleExponentialMovingAverageOscillator::DoubleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0),
      ema1_(0.0), ema2_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    
    // Set minperiod
    _minperiod(getMinPeriod());
    
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
    }
}

DoubleExponentialMovingAverageOscillator::DoubleExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source)
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema1_(0.0), ema2_(0.0), first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    
    // Set minperiod
    _minperiod(getMinPeriod());
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
        this->data_source_ = lineseries;
    }
}

DoubleExponentialMovingAverageOscillator::DoubleExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema1_(0.0), ema2_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    
    // Set minperiod
    _minperiod(getMinPeriod());
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
        this->data_source_ = lineseries;
    }
}

void DoubleExponentialMovingAverageOscillator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double DoubleExponentialMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto demaosc_line = lines->getline(demaosc);
    if (!demaosc_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // LineBuffer already correctly implements Python-style indexing
    // Just pass the ago value directly
    return (*demaosc_line)[ago];
}

int DoubleExponentialMovingAverageOscillator::getMinPeriod() const {
    // DEMA minimum period is 2 * period - 1
    // With default period 30, this should be 59
    return 2 * params.period - 1;
}

size_t DoubleExponentialMovingAverageOscillator::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto demaosc_line = lines->getline(demaosc);
    return demaosc_line ? demaosc_line->size() : 0;
}

void DoubleExponentialMovingAverageOscillator::calculate() {
    if (datas.empty() || !datas[0]) return;
    
    // Use getDataSize utility which handles LineBuffer _idx=-1 case
    size_t data_size = utils::getDataSize(datas[0]);
    if (data_size == 0) return;
    
    // Use once() to calculate all values at once
    once(0, static_cast<int>(data_size));
}

void DoubleExponentialMovingAverageOscillator::next() {
    calculate();
}

void DoubleExponentialMovingAverageOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Use getDataLine utility to get close price
    auto data_line = utils::getDataLine(datas[0]);
    if (!data_line) {
        return;
    }
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) {
        return;
    }
    
    auto demaosc_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(demaosc));
    if (!demaosc_buffer) {
        return;
    }
    
    // Clear the buffer and reset
    demaosc_buffer->reset();
    
    // Get the raw array for direct access
    const auto& data_array = data_buffer->array();
    
    // Determine effective size
    size_t effective_size = std::min(static_cast<size_t>(end), data_array.size());
    if (effective_size == 0) return;
    
    int min_required = getMinPeriod(); // 2 * period - 1
    
    // Calculate all EMA1 values first
    std::vector<double> ema1_values;
    ema1_values.reserve(effective_size);
    
    double ema1 = 0.0;
    bool ema1_initialized = false;
    int first_valid_idx = -1;
    
    // Find the first non-NaN value
    for (size_t i = 0; i < effective_size; ++i) {
        if (!std::isnan(data_array[i])) {
            first_valid_idx = i;
            break;
        }
    }
    
    if (first_valid_idx < 0) return; // All data is NaN
    
    for (size_t i = 0; i < effective_size; ++i) {
        double price = data_array[i];
        
        if (std::isnan(price)) {
            ema1_values.push_back(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        // Check if we have enough valid data points from first_valid_idx
        if (i < static_cast<size_t>(first_valid_idx + params.period - 1)) {
            ema1_values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else if (i == static_cast<size_t>(first_valid_idx + params.period - 1)) {
            // Calculate initial SMA for EMA1
            double sum = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum += data_array[first_valid_idx + j];
            }
            ema1 = sum / params.period;
            ema1_values.push_back(ema1);
            ema1_initialized = true;
        } else if (ema1_initialized) {
            ema1 = alpha_ * price + alpha1_ * ema1;
            ema1_values.push_back(ema1);
        } else {
            ema1_values.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Calculate all EMA2 values from EMA1
    std::vector<double> ema2_values;
    ema2_values.reserve(ema1_values.size());
    
    double ema2 = 0.0;
    bool ema2_initialized = false;
    
    for (size_t i = 0; i < ema1_values.size(); ++i) {
        // EMA2 needs to wait for enough EMA1 values
        if (i < static_cast<size_t>(first_valid_idx + params.period - 1)) {
            ema2_values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else if (i < static_cast<size_t>(first_valid_idx + 2 * params.period - 2)) {
            ema2_values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else if (i == static_cast<size_t>(first_valid_idx + 2 * params.period - 2)) {
            // Calculate initial SMA of EMA1 values
            double sum = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum += ema1_values[i - params.period + 1 + j];
            }
            ema2 = sum / params.period;
            ema2_values.push_back(ema2);
            ema2_initialized = true;
        } else if (ema2_initialized && !std::isnan(ema1_values[i])) {
            ema2 = alpha_ * ema1_values[i] + alpha1_ * ema2;
            ema2_values.push_back(ema2);
        } else {
            ema2_values.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Calculate DEMA oscillator values
    for (size_t i = 0; i < effective_size; ++i) {
        if (i < static_cast<size_t>(min_required) || 
            std::isnan(ema1_values[i]) || std::isnan(ema2_values[i])) {
            demaosc_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            double dema = 2.0 * ema1_values[i] - ema2_values[i];
            double oscillator = data_array[i] - dema;
            demaosc_buffer->append(oscillator);
        }
    }
    
    // Sync buffer index with data buffer
    if (demaosc_buffer->size() > 0) {
        // Set index to the last valid position
        demaosc_buffer->set_idx(demaosc_buffer->size() - 1);
    }
}

} // namespace indicators
} // namespace backtrader