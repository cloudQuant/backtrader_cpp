#include "indicators/dv2.h"
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

// DV2 implementation
DV2::DV2() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period + params.maperiod - 1);
    
    // Create component indicators
    sma_ = std::make_shared<SMA>();
    
    percent_rank_ = std::make_shared<PercentRank>();
}

DV2::DV2(std::shared_ptr<LineRoot> data, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + params.maperiod - 1);
    
    // Create component indicators
    sma_ = std::make_shared<SMA>();
    
    percent_rank_ = std::make_shared<PercentRank>();
}

double DV2::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(dv2);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int DV2::getMinPeriod() const {
    return params.period + params.maperiod - 1;
}

void DV2::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void DV2::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void DV2::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    auto dv2_line = lines->getline(dv2);
    
    if (!dv2_line) return;
    
    // Get OHLC data
    double high = (*data_lines->getline(1))[0];   // High
    double low = (*data_lines->getline(2))[0];    // Low
    double close = (*data_lines->getline(3))[0];  // Close
    
    // Calculate CHL ratio: close / ((high + low) / 2)
    double hl_avg = (high + low) / 2.0;
    double chl = (hl_avg != 0.0) ? close / hl_avg : 1.0;
    
    // Store CHL value
    chl_values_.push_back(chl);
    
    // Calculate DVU (moving average of CHL)
    if (chl_values_.size() >= params.maperiod) {
        // Calculate SMA of last maperiod CHL values
        double sum = 0.0;
        int start_idx = std::max(0, static_cast<int>(chl_values_.size()) - params.maperiod);
        for (int i = start_idx; i < chl_values_.size(); ++i) {
            sum += chl_values_[i];
        }
        double dvu = sum / params.maperiod;
        dvu_values_.push_back(dvu);
        
        // Calculate percent rank of DVU if we have enough values
        if (dvu_values_.size() >= params.period) {
            // Get the period data for percent rank calculation
            std::vector<double> period_data;
            int pr_start_idx = std::max(0, static_cast<int>(dvu_values_.size()) - params.period);
            for (int i = pr_start_idx; i < dvu_values_.size(); ++i) {
                period_data.push_back(dvu_values_[i]);
            }
            
            // Calculate percent rank
            double current_dvu = dvu_values_.back();
            int count_less = 0;
            for (double value : period_data) {
                if (value < current_dvu) {
                    count_less++;
                }
            }
            
            double percent_rank = static_cast<double>(count_less) / period_data.size();
            dv2_line->set(0, percent_rank * 100.0);  // Convert to percentage
        } else {
            dv2_line->set(0, 50.0);  // Default value when not enough data
        }
    } else {
        dv2_line->set(0, 50.0);  // Default value when not enough data
    }
    
    // Keep only necessary history
    if (chl_values_.size() > params.maperiod + params.period) {
        chl_values_.erase(chl_values_.begin());
    }
    if (dvu_values_.size() > params.period) {
        dvu_values_.erase(dvu_values_.begin());
    }
}

void DV2::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    auto dv2_line = lines->getline(dv2);
    
    if (!dv2_line) return;
    
    // Clear previous calculations
    chl_values_.clear();
    dvu_values_.clear();
    
    // Calculate CHL values for all bars
    std::vector<double> all_chl_values;
    for (int i = 0; i < end; ++i) {
        double high = (*data_lines->getline(1))[i];   // High
        double low = (*data_lines->getline(2))[i];    // Low
        double close = (*data_lines->getline(3))[i];  // Close
        
        double hl_avg = (high + low) / 2.0;
        double chl = (hl_avg != 0.0) ? close / hl_avg : 1.0;
        all_chl_values.push_back(chl);
    }
    
    // Calculate DVU values
    std::vector<double> all_dvu_values;
    for (int i = params.maperiod - 1; i < all_chl_values.size(); ++i) {
        double sum = 0.0;
        for (int j = 0; j < params.maperiod; ++j) {
            sum += all_chl_values[i - j];
        }
        double dvu = sum / params.maperiod;
        all_dvu_values.push_back(dvu);
    }
    
    // Calculate DV2 values
    for (int i = start; i < end; ++i) {
        int dvu_index = i - params.maperiod + 1;
        
        if (dvu_index >= 0 && dvu_index < all_dvu_values.size() && 
            dvu_index >= params.period - 1) {
            
            // Get period data for percent rank
            std::vector<double> period_data;
            int pr_start = std::max(0, dvu_index - params.period + 1);
            for (int j = pr_start; j <= dvu_index; ++j) {
                period_data.push_back(all_dvu_values[j]);
            }
            
            // Calculate percent rank
            double current_dvu = all_dvu_values[dvu_index];
            int count_less = 0;
            for (double value : period_data) {
                if (value < current_dvu) {
                    count_less++;
                }
            }
            
            double percent_rank = static_cast<double>(count_less) / period_data.size();
            dv2_line->set(i, percent_rank * 100.0);  // Convert to percentage
        } else {
            dv2_line->set(i, 50.0);  // Default value when not enough data
        }
    }
}

} // namespace indicators
} // namespace backtrader