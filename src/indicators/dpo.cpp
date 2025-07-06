#include "indicators/dpo.h"
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

// DetrendedPriceOscillator implementation
DetrendedPriceOscillator::DetrendedPriceOscillator() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
}

DetrendedPriceOscillator::DetrendedPriceOscillator(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
}

DetrendedPriceOscillator::DetrendedPriceOscillator(std::shared_ptr<LineRoot> data) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
}

DetrendedPriceOscillator::DetrendedPriceOscillator(std::shared_ptr<LineRoot> data, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
}

double DetrendedPriceOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(dpo);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int DetrendedPriceOscillator::getMinPeriod() const {
    return params.period;
}

void DetrendedPriceOscillator::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void DetrendedPriceOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void DetrendedPriceOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto dpo_line = lines->getline(dpo);
    
    if (!data_line || !dpo_line) return;
    
    // Calculate DPO value
    double dpo_value = calculate_dpo(0);
    dpo_line->set(0, dpo_value);
}

void DetrendedPriceOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto dpo_line = lines->getline(dpo);
    
    if (!data_line || !dpo_line) return;
    
    for (int i = start; i < end; ++i) {
        // Calculate DPO value for index i
        double dpo_value = calculate_dpo(i);
        dpo_line->set(i, dpo_value);
    }
}

double DetrendedPriceOscillator::calculate_dpo(int index) {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line) return 0.0;
    
    // Get current price
    double current_price = (*data_line)[index];
    
    // Calculate moving average lookback offset
    int lookback = params.period / 2 + 1;
    
    // Calculate SMA for the shifted period
    double ma_sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        ma_sum += (*data_line)[index - lookback - i];
    }
    double ma_value = ma_sum / params.period;
    
    // DPO = current price - shifted moving average
    return current_price - ma_value;
}

} // namespace indicators
} // namespace backtrader