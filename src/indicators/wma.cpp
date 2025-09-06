#include "indicators/wma.h"
#include <numeric>
#include <limits>

namespace backtrader {
namespace indicators {

// WeightedMovingAverage implementation
WeightedMovingAverage::WeightedMovingAverage() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
    
    // Calculate coefficient and weights
    coef_ = 2.0 / (params.period * (params.period + 1.0));
    
    // Create weights: 1, 2, 3, ..., period
    weights_.resize(params.period);
    for (int i = 0; i < params.period; ++i) {
        weights_[i] = static_cast<double>(i + 1);
    }
}

WeightedMovingAverage::WeightedMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Set data member for proper indicator functioning
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Calculate coefficient and weights
    coef_ = 2.0 / (params.period * (params.period + 1.0));
    
    // Create weights: 1, 2, 3, ..., period
    weights_.resize(params.period);
    for (int i = 0; i < params.period; ++i) {
        weights_[i] = static_cast<double>(i + 1);
    }
}

WeightedMovingAverage::WeightedMovingAverage(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Set data member for proper indicator functioning
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Calculate coefficient and weights
    coef_ = 2.0 / (params.period * (params.period + 1.0));
    
    // Create weights: 1, 2, 3, ..., period
    weights_.resize(params.period);
    for (int i = 0; i < params.period; ++i) {
        weights_[i] = static_cast<double>(i + 1);
    }
}

void WeightedMovingAverage::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void WeightedMovingAverage::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto wma_line = lines->getline(wma);
    
    if (!data_line || !wma_line) return;
    
    // Calculate weighted sum
    double weighted_sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        // weight[i] * data[period - i - 1]
        // Newest data gets highest weight
        weighted_sum += weights_[i] * (*data_line)[-(params.period - i - 1)];
    }
    
    // Apply coefficient
    wma_line->set(0, coef_ * weighted_sum);
}

void WeightedMovingAverage::once(int start, int end) {
    // Use data_source_ if available, otherwise use data
    auto data_to_use = data_source_ ? data_source_ : data;
    
    if (!data_to_use || !data_to_use->lines || data_to_use->lines->size() == 0) {
        return;
    }
    
    // Use the close price line (index 3) for OHLC data, or primary line (index 0) for simple data
    auto data_line = data_to_use->lines->getline(data_to_use->lines->size() > 4 ? 4 : 0);
    auto wma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(wma));
    
    if (!data_line || !wma_line) return;
    
    // Clear the WMA line buffer and reset to initial state
    wma_line->reset();
    
    // Get the actual data array for direct access
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    const auto& data_array = data_buffer->array();
    
    // Process data in forward order (matching Python's behavior)
    for (int i = start; i < end; ++i) {
        if (i < params.period - 1) {
            // Not enough data for calculation
            wma_line->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        // Calculate weighted sum for position i using the window [i-period+1, i]
        double weighted_sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            // Get value from the data array directly
            // Note: array[0] is NaN, actual data starts at array[1]
            int data_idx = (i + 1) - (params.period - 1 - j);
            if (data_idx < 1 || data_idx >= static_cast<int>(data_array.size())) {
                continue;
            }
            double value = data_array[data_idx];
            // weights_[j] = j+1, so oldest data gets weight 1, newest gets weight period
            weighted_sum += weights_[j] * value;
        }
        
        // Apply coefficient and append value
        double wma_value = coef_ * weighted_sum;
        wma_line->append(wma_value);
    }
}

double WeightedMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto wma_line = lines->getline(0);
    if (!wma_line || wma_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // In backtrader, ago=0 means current value, ago=-1 means one bar ago
    // Convert to array index: ago=0 -> last element, ago=-1 -> second to last, etc.
    int index;
    if (ago <= 0) {
        index = static_cast<int>(wma_line->size()) - 1 + ago;
    } else {
        // Positive ago is unusual but handle it
        index = static_cast<int>(wma_line->size()) - 1 - ago;
    }
    
    if (index < 0 || index >= static_cast<int>(wma_line->size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*wma_line)[index];
}

size_t WeightedMovingAverage::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    
    auto wma_line = lines->getline(0);
    if (!wma_line) {
        return 0;
    }
    
    return wma_line->size();
}

void WeightedMovingAverage::calculate() {
    if (data_source_) {
        // For data source constructor, calculate for entire dataset
        if (data_source_->lines && data_source_->lines->size() > 0) {
            auto data_line = data_source_->lines->getline(0);
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (data_buffer) {
                // Use the array size minus 1 (since array[0] is NaN)
                once(0, data_buffer->array().size() - 1);
            }
        }
    } else if (data && data->lines && data->lines->size() > 0) {
        // For test framework constructor, calculate for entire dataset
        auto data_line = data->lines->getline(data->lines->size() > 4 ? 4 : 0);
        auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
        if (data_buffer) {
            // Use the array size minus 1 (since array[0] is NaN)
            once(0, data_buffer->array().size() - 1);
        }
    } else {
        // For normal constructor, use the existing next() logic
        next();
    }
}

} // namespace indicators
} // namespace backtrader