#include "indicators/emaosc.h"
#include "linebuffer.h"
#include <cmath>
#include <cstdio>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator() 
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema_(0.0), first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0),
      ema_(0.0), first_run_(true) {
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0),
      ema_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source)
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema_(0.0), first_run_(true) {
    printf("DEBUG: EMAOsc DataSeries constructor: params.period = %d\n", params.period);
    fflush(stdout);
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

ExponentialMovingAverageOscillator::ExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator(), data_source_(nullptr), current_index_(0),
      ema_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

void ExponentialMovingAverageOscillator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double ExponentialMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return 0.0;
    }
    
    auto emaosc_line = lines->getline(emaosc);
    if (!emaosc_line) {
        return 0.0;
    }
    
    // Use the ago parameter directly (already negative for past values)
    return (*emaosc_line)[ago];
}

int ExponentialMovingAverageOscillator::getMinPeriod() const {
    // Debug output
    fprintf(stderr, "ExponentialMovingAverageOscillator getMinPeriod: params.period = %d\n", params.period);
    fflush(stderr);
    return params.period;
}

void ExponentialMovingAverageOscillator::calculate() {
    if (!lines || lines->size() == 0) {
        setup_lines();
    }
    
    auto emaosc_line = lines->getline(emaosc);
    if (!emaosc_line) {
        return;
    }
    
    // Get data source - try both DataSeries and LineSeries
    std::shared_ptr<LineSingle> data_line;
    if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 4) {
        // Use close price (line 4) for DataSeries
        data_line = datas[0]->lines->getline(4);
    } else if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        // Use first line for LineSeries
        data_line = datas[0]->lines->getline(0);
    }
    
    if (!data_line || data_line->size() == 0) {
        return;
    }
    
    // Use once() method to calculate all values (following SMA pattern)
    once(0, data_line->size());
}

void ExponentialMovingAverageOscillator::next() {
    calculate();
}

void ExponentialMovingAverageOscillator::once(int start, int end) {
    if (!datas.empty() && datas[0] && datas[0]->lines) {
        // Get data source - try both DataSeries and LineSeries  
        std::shared_ptr<LineSingle> data_line;
        if (datas[0]->lines->size() > 4) {
            // Use close price (line 4) for DataSeries
            data_line = datas[0]->lines->getline(4);
        } else if (datas[0]->lines->size() > 0) {
            // Use first line for LineSeries
            data_line = datas[0]->lines->getline(0);
        }
        
        if (!data_line) {
            return;
        }
        
        auto emaosc_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(emaosc));
        if (!emaosc_buffer) {
            return;
        }
        
        // Get data as LineBuffer for direct array access
        auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
        if (!data_buffer) {
            return;
        }
        
        const auto& data_array = data_buffer->array();
        
        // Clear the buffer
        emaosc_buffer->reset();
        
        // Find where actual data starts (skip initial NaNs) 
        int data_start = 0;
        if (!data_array.empty() && std::isnan(data_array[0])) {
            data_start = 1;
        }
        
        // Calculate for the entire data range
        int effective_size = static_cast<int>(data_array.size());
        
        // Process all data points to match the input data size exactly
        // Need to create exactly effective_size elements to match input
        
        // Calculate EMA oscillator values for all positions
        for (int i = 0; i < effective_size; ++i) {
            if (i < data_start + params.period - 1) {
                // Not enough data for EMA calculation
                emaosc_buffer->append(std::numeric_limits<double>::quiet_NaN());
            } else if (i == data_start + params.period - 1) {
                // First EMA value - use SMA as seed
                double sma_sum = 0.0;
                for (int j = 0; j < params.period; ++j) {
                    sma_sum += data_array[data_start + j];
                }
                double sma_seed = sma_sum / params.period;
                
                // First oscillator value = price - SMA
                double first_price = data_array[i];
                double first_osc = first_price - sma_seed;
                emaosc_buffer->append(first_osc);
                
                // Initialize EMA state for subsequent calculations
                ema_ = sma_seed;
                first_run_ = false;
            } else {
                // Subsequent EMA values
                double price = data_array[i];
                // Calculate EMA: EMA_today = (price * alpha) + (EMA_yesterday * (1-alpha))
                ema_ = alpha_ * price + alpha1_ * ema_;
                // Calculate oscillator: price - EMA
                double osc_value = price - ema_;
                emaosc_buffer->append(osc_value);
            }
        }
        
        // Set buffer index to last position for proper ago indexing
        // The buffer should now have exactly effective_size elements
        if (emaosc_buffer->size() > 0) {
            emaosc_buffer->set_idx(emaosc_buffer->size() - 1);
        }
    }
}

size_t ExponentialMovingAverageOscillator::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto emaosc_line = lines->getline(emaosc);
    return emaosc_line ? emaosc_line->size() : 0;
}

} // namespace indicators
} // namespace backtrader