#include "indicators/percentrank.h"
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

// PercentRank implementation
PercentRank::PercentRank() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
}

PercentRank::PercentRank(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
}

double PercentRank::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(pctrank);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int PercentRank::getMinPeriod() const {
    return params.period;
}

void PercentRank::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void PercentRank::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void PercentRank::prenext() {
    Indicator::prenext();
}

void PercentRank::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto pctrank_line = lines->getline(pctrank);
    
    if (!data_line || !pctrank_line) return;
    
    double current_value = (*data_line)[0];
    
    // Add current value to period data
    period_data_.push_back(current_value);
    
    // Keep only the period we need
    if (period_data_.size() > params.period) {
        period_data_.erase(period_data_.begin());
    }
    
    // Calculate percent rank if we have enough data
    if (period_data_.size() >= params.period) {
        double pct_rank = calculate_percent_rank(period_data_, current_value);
        pctrank_line->set(0, pct_rank);
    } else {
        pctrank_line->set(0, 0.0);
    }
}

void PercentRank::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto pctrank_line = lines->getline(pctrank);
    
    if (!data_line || !pctrank_line) return;
    
    for (int i = start; i < end; ++i) {
        double current_value = (*data_line)[i];
        
        // Build period data for current position
        std::vector<double> current_period_data;
        int data_start = std::max(0, i - params.period + 1);
        
        for (int j = data_start; j <= i; ++j) {
            current_period_data.push_back((*data_line)[j]);
        }
        
        // Calculate percent rank if we have enough data
        if (current_period_data.size() >= params.period) {
            double pct_rank = calculate_percent_rank(current_period_data, current_value);
            pctrank_line->set(i, pct_rank);
        } else {
            pctrank_line->set(i, 0.0);
        }
    }
}

double PercentRank::calculate_percent_rank(const std::vector<double>& data, double current_value) {
    if (data.empty()) return 0.0;
    
    // Count how many values are less than the current value
    int count_less = 0;
    for (double value : data) {
        if (value < current_value) {
            count_less++;
        }
    }
    
    // Percent rank = count of values less than current / total count
    return static_cast<double>(count_less) / data.size();
}

} // namespace indicators
} // namespace backtrader