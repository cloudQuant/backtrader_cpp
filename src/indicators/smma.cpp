#include "indicators/smma.h"
#include <numeric>
#include <limits>

namespace backtrader {
namespace indicators {

// SmoothedMovingAverage implementation
SmoothedMovingAverage::SmoothedMovingAverage() 
    : Indicator(), prev_smma_(0.0), initialized_(false), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 1.0 / params.period;
    alpha1_ = 1.0 - alpha_;
    
    _minperiod(params.period);
}

SmoothedMovingAverage::SmoothedMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), prev_smma_(0.0), initialized_(false), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 1.0 / params.period;
    alpha1_ = 1.0 - alpha_;
    
    _minperiod(params.period);
}

SmoothedMovingAverage::SmoothedMovingAverage(std::shared_ptr<LineRoot> data, int period) 
    : Indicator(), prev_smma_(0.0), initialized_(false), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate smoothing factors
    alpha_ = 1.0 / params.period;
    alpha1_ = 1.0 - alpha_;
    
    _minperiod(params.period);
}

double SmoothedMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(smma);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int SmoothedMovingAverage::getMinPeriod() const {
    return params.period;
}

void SmoothedMovingAverage::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void SmoothedMovingAverage::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void SmoothedMovingAverage::prenext() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line) return;
    
    // Collect seed values for initial calculation
    seed_values_.push_back((*data_line)[0]);
    
    // Keep only the period we need
    if (seed_values_.size() > params.period) {
        seed_values_.erase(seed_values_.begin());
    }
    
    Indicator::prenext();
}

void SmoothedMovingAverage::nextstart() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto smma_line = lines->getline(smma);
    if (!smma_line) return;
    
    // Calculate initial SMMA as simple average of first period values
    if (seed_values_.size() >= params.period) {
        double sum = std::accumulate(seed_values_.begin(), seed_values_.end(), 0.0);
        prev_smma_ = sum / params.period;
        smma_line->set(0, prev_smma_);
        initialized_ = true;
    }
}

void SmoothedMovingAverage::next() {
    if (!initialized_) {
        nextstart();
        return;
    }
    
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto smma_line = lines->getline(smma);
    
    if (!data_line || !smma_line) return;
    
    double current_data = (*data_line)[0];
    
    // SMMA formula: new_value = (old_value * (period - 1) + new_data) / period
    // Which is equivalent to: new_value = old_value * alpha1 + new_data * alpha
    double smma_value = prev_smma_ * alpha1_ + current_data * alpha_;
    
    smma_line->set(0, smma_value);
    prev_smma_ = smma_value;
}

void SmoothedMovingAverage::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto smma_line = lines->getline(smma);
    
    if (!data_line || !smma_line) return;
    
    // Calculate initial SMMA as simple average of first period values
    double sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        sum += (*data_line)[start + i];
    }
    double smma = sum / params.period;
    smma_line->set(start + params.period - 1, smma);
    
    // Calculate subsequent SMMA values using exponential smoothing
    for (int i = start + params.period; i < end; ++i) {
        double current_data = (*data_line)[i];
        smma = smma * alpha1_ + current_data * alpha_;
        smma_line->set(i, smma);
    }
}

} // namespace indicators
} // namespace backtrader