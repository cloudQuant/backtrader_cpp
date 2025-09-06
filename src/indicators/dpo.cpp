#include "indicators/dpo.h"
#include "dataseries.h"
#include <algorithm>
#include <limits>
#include <iostream>
#include <cmath>

namespace backtrader {
namespace indicators {

// DetrendedPriceOscillator implementation
DetrendedPriceOscillator::DetrendedPriceOscillator() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    // DPO needs period + (period // 2) - 1
    // For period=20: minperiod = 20 + 10 - 1 = 29
    _minperiod(params.period + (params.period / 2) - 1);
}

DetrendedPriceOscillator::DetrendedPriceOscillator(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    // DPO needs period + (period // 2) - 1
    // For period=14: minperiod = 14 + 7 - 1 = 20
    _minperiod(params.period + (params.period / 2) - 1);
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Calculate immediately upon construction if we have data
    if (data_source && data_source->lines && data_source->lines->size() > 0) {
        calculate();
    }
}

DetrendedPriceOscillator::DetrendedPriceOscillator(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    // DPO needs period + (period // 2) - 1
    _minperiod(params.period + (params.period / 2) - 1);
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
    
    // Calculate immediately upon construction if we have data
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        calculate();
    }
}

DetrendedPriceOscillator::DetrendedPriceOscillator(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.period = period;
    setup_lines();
    // DPO needs period + (period // 2) - 1
    _minperiod(params.period + (params.period / 2) - 1);
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
    
    // Calculate immediately upon construction if we have data
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        calculate();
    }
}

double DetrendedPriceOscillator::get(int ago) const {
    // Use the base class implementation which uses lines_
    return IndicatorBase::get(ago);
}

size_t DetrendedPriceOscillator::size() const {
    // Use the base class implementation which uses lines_
    return IndicatorBase::size();
}

void DetrendedPriceOscillator::calculate() {
    if (!data || !data->lines || data->lines->size() == 0) {
        // Try with datas vector if data is not set
        if (datas.empty() || !datas[0] || !datas[0]->lines || datas[0]->lines->size() == 0) {
            return;
        }
        data = datas[0];
    }
    
    // Get close price line - for DataSeries it's index 3, for simple LineSeries it's index 0
    int close_idx = 0;
    if (data->lines->size() > 4) {
        close_idx = 3;  // DataSeries::Close
    }
    auto data_line = data->lines->getline(close_idx);
    if (!data_line) {
        return;
    }
    
    auto dpo_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!dpo_line) {
        return;
    }
    
    // Clear and recalculate all values only if not already calculated
    // LineBuffer starts with 1 NaN value, so check if we have more than 1 value
    if (dpo_line->size() <= 1) {
        dpo_line->reset();
    } else {
        return;  // Already calculated
    }
    
    // Use once() method to calculate all values at correct positions
    int data_size = static_cast<int>(data_line->size());
    once(0, data_size);
    
    // Sync lines to lines_ after calculation
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
}

void DetrendedPriceOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    
    // Sync lines to lines_ for IndicatorBase compatibility
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
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
    // Use data member if available, otherwise use datas[0]
    auto data_source = data ? data : (!datas.empty() ? datas[0] : nullptr);
    if (!data_source || !data_source->lines || data_source->lines->size() == 0) {
        return;
    }
    
    // Get close price line - for DataSeries it's index 3, for simple LineSeries it's index 0
    int close_idx = 0;
    if (data_source->lines->size() > 4) {
        close_idx = 3;  // DataSeries::Close
    }
    auto data_line = data_source->lines->getline(close_idx);
    auto dpo_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(dpo));
    
    if (!data_line || !dpo_line) {
        return;
    }
    
    // Cast to LineBuffer to access raw data
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) {
        return;
    }
    
    int data_size = static_cast<int>(data_buffer->size());
    int min_period = params.period + (params.period / 2) - 1;
    
    // Calculate DPO for all data points chronologically
    for (int i = start; i < end && i < data_size; ++i) {
        if (i < min_period) {
            // Not enough data yet
            dpo_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Current price at chronological position i
            double current_price = data_buffer->array()[i];
            
            // Calculate shifted SMA
            // Python: ma(-period // 2 + 1) means we use SMA from (period//2 - 1) bars before current
            int shift = params.period / 2 - 1;
            int ma_center_pos = i - shift;  // Position where MA is centered
            
            if (ma_center_pos >= params.period - 1) {
                // Calculate SMA centered at ma_center_pos
                double ma_sum = 0.0;
                for (int j = 0; j < params.period; ++j) {
                    int data_pos = ma_center_pos - params.period + 1 + j;
                    if (data_pos >= 0 && data_pos < data_size) {
                        ma_sum += data_buffer->array()[data_pos];
                    }
                }
                double ma_value = ma_sum / params.period;
                
                // DPO = current price - shifted moving average
                double dpo_value = current_price - ma_value;
                dpo_line->append(dpo_value);
            } else {
                dpo_line->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Set the index to the last element (most recent)
    dpo_line->set_idx(dpo_line->size() - 1);
}

double DetrendedPriceOscillator::calculate_dpo(int index) {
    if (datas.empty() || !datas[0]->lines) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Get current price
    double current_price = (*data_line)[index];
    
    // Python formula: close - ma(-period // 2 + 1)
    // lookback = -(period // 2) + 1 (negative means look back)
    int lookback = -(params.period / 2) + 1;  // For period=20: -10+1=-9
    
    // Calculate SMA for the shifted period (centered around lookback position)
    int ma_end_index = index + lookback;  // End position for MA calculation
    
    // Check bounds
    if (ma_end_index - params.period + 1 < 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double ma_sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        int data_idx = ma_end_index - i;
        if (data_idx < 0 || data_idx >= static_cast<int>(data_line->size())) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        ma_sum += (*data_line)[data_idx];
    }
    double ma_value = ma_sum / params.period;
    
    // DPO = current price - shifted moving average
    return current_price - ma_value;
}

} // namespace indicators
} // namespace backtrader