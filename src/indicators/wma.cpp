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
    auto wma_line = lines->getline(Lines::wma);
    
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
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto wma_line = lines->getline(Lines::wma);
    
    if (!data_line || !wma_line) return;
    
    for (int i = start; i < end; ++i) {
        // Calculate weighted sum
        double weighted_sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            // weight[j] * data[i - (period - j - 1)]
            // Newest data gets highest weight
            weighted_sum += weights_[j] * (*data_line)[i - (params.period - j - 1)];
        }
        
        // Apply coefficient
        wma_line->set(i, coef_ * weighted_sum);
    }
}

double WeightedMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto wma_line = lines->getline(0);
    if (!wma_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*wma_line)[ago];
}

void WeightedMovingAverage::calculate() {
    if (!data_source_ || !data_source_->lines || data_source_->lines->size() == 0) {
        return;
    }
    
    auto data_line = data_source_->lines->getline(0);
    
    if (!data_line) {
        return;
    }
    
    // Calculate WMA for the entire dataset using once() method
    once(0, data_line->size());
}

} // namespace indicators
} // namespace backtrader