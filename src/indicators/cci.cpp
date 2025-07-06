#include "indicators/cci.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>

namespace backtrader {
namespace indicators {

// CommodityChannelIndex implementation
CommodityChannelIndex::CommodityChannelIndex() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Create SMA for typical price
    tp_sma_ = std::make_shared<SMA>();
    
    _minperiod(params.period);
}

CommodityChannelIndex::CommodityChannelIndex(std::shared_ptr<LineRoot> data) : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Create SMA for typical price
    tp_sma_ = std::make_shared<SMA>();
    
    _minperiod(params.period);
    // This constructor is for test framework compatibility
}

CommodityChannelIndex::CommodityChannelIndex(std::shared_ptr<LineSeries> data_source, int period, double factor) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    params.factor = factor;
    
    setup_lines();
    
    // Create SMA for typical price
    tp_sma_ = std::make_shared<SMA>();
    
    _minperiod(params.period);
}

void CommodityChannelIndex::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void CommodityChannelIndex::prenext() {
    Indicator::prenext();
}

void CommodityChannelIndex::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto cci_line = lines->getline(cci);
    if (!cci_line) return;
    
    // Calculate typical price for current bar
    double tp = calculate_typical_price();
    
    // Store typical price value
    tp_values_.push_back(tp);
    if (tp_values_.size() > params.period) {
        tp_values_.erase(tp_values_.begin());
    }
    
    // Need full period for calculation
    if (tp_values_.size() < params.period) {
        cci_line->set(0, 0.0);
        return;
    }
    
    // Calculate mean of typical price
    double tp_mean = std::accumulate(tp_values_.begin(), tp_values_.end(), 0.0) / params.period;
    
    // Calculate deviation
    double deviation = tp - tp_mean;
    
    // Calculate mean deviation
    double mean_deviation = calculate_mean_deviation(tp_values_, tp_mean);
    
    // Calculate CCI
    if (mean_deviation != 0.0) {
        cci_line->set(0, deviation / (params.factor * mean_deviation));
    } else {
        cci_line->set(0, 0.0);
    }
}

void CommodityChannelIndex::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto cci_line = lines->getline(cci);
    if (!cci_line) return;
    
    // Build typical price values for the range
    std::vector<double> all_tp_values;
    for (int i = start - params.period + 1; i < end; ++i) {
        double tp = calculate_typical_price(start - i);
        all_tp_values.push_back(tp);
    }
    
    for (int i = start; i < end; ++i) {
        // Get typical price values for current window
        std::vector<double> current_tp_values;
        int start_idx = i - start + params.period - 1;
        
        for (int j = 0; j < params.period; ++j) {
            if (start_idx - j >= 0 && start_idx - j < all_tp_values.size()) {
                current_tp_values.push_back(all_tp_values[start_idx - j]);
            }
        }
        
        if (current_tp_values.size() < params.period) {
            cci_line->set(i, 0.0);
            continue;
        }
        
        // Calculate mean of typical price
        double tp_mean = std::accumulate(current_tp_values.begin(), current_tp_values.end(), 0.0) / params.period;
        
        // Current typical price
        double current_tp = calculate_typical_price(start - i);
        
        // Calculate deviation
        double deviation = current_tp - tp_mean;
        
        // Calculate mean deviation
        double mean_deviation = calculate_mean_deviation(current_tp_values, tp_mean);
        
        // Calculate CCI
        if (mean_deviation != 0.0) {
            cci_line->set(i, deviation / (params.factor * mean_deviation));
        } else {
            cci_line->set(i, 0.0);
        }
    }
}

double CommodityChannelIndex::calculate_typical_price(int offset) {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    auto low_line = datas[0]->lines->getline(2);  // Low line
    auto close_line = datas[0]->lines->getline(3); // Close line
    
    if (!high_line || !low_line || !close_line) return 0.0;
    
    double high = (*high_line)[-offset];
    double low = (*low_line)[-offset];
    double close = (*close_line)[-offset];
    
    return (high + low + close) / 3.0;
}

double CommodityChannelIndex::calculate_mean_deviation(const std::vector<double>& tp_values, double mean) {
    if (tp_values.empty()) return 0.0;
    
    double sum_deviation = 0.0;
    for (double value : tp_values) {
        sum_deviation += std::abs(value - mean);
    }
    
    return sum_deviation / tp_values.size();
}

double CommodityChannelIndex::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto cci_line = lines->getline(0);
    if (!cci_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*cci_line)[ago];
}

void CommodityChannelIndex::calculate() {
    if (!data_source_ || !data_source_->lines || data_source_->lines->size() == 0) {
        return;
    }
    
    auto data_line = data_source_->lines->getline(0);
    
    if (!data_line) {
        return;
    }
    
    // Calculate CCI for the entire dataset using once() method
    once(0, data_line->size());
}

} // namespace indicators
} // namespace backtrader