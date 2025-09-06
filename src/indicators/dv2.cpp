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

DV2::DV2(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + params.maperiod - 1);
    
    // Cast DataSeries to LineSeries for compatibility
    auto lineseries = std::static_pointer_cast<LineSeries>(data_source);
    data_source_ = lineseries;
    this->data = lineseries;
    this->datas.push_back(lineseries);
    
    // Create component indicators
    sma_ = std::make_shared<SMA>();
    percent_rank_ = std::make_shared<PercentRank>();
}

double DV2::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(dv2);
    if (!line || line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use LineBuffer's operator[] which handles ago indexing
    return (*line)[ago];
}

int DV2::getMinPeriod() const {
    return params.period + params.maperiod - 1;
}

size_t DV2::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto dv2_line = lines->getline(dv2);
    if (!dv2_line) {
        return 0;
    }
    return dv2_line->size();
}

void DV2::calculate() {
    if (data_source_) {
        // For DataSeries constructor, calculate for entire dataset
        if (data_source_->lines && data_source_->lines->size() > 0) {
            // Get close line for DV2 calculation (index 3 for DataSeries)
            auto close_line = data_source_->lines->getline(3);
            if (close_line) {
                size_t data_size = close_line->size();
                datas.push_back(data_source_);  // Ensure datas is populated
                once(0, data_size);
            }
        }
    } else if (datas.size() > 0 && datas[0] && datas[0]->lines) {
        // Alternative path using datas
        auto close_line = datas[0]->lines->getline(3);
        if (close_line) {
            size_t data_size = close_line->size();
            once(0, data_size);
        }
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
    
    // Get OHLC data (DataSeries uses indices: 0=Open, 1=High, 2=Low, 3=Close, 4=Volume)
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
            dv2_line->set(0, std::numeric_limits<double>::quiet_NaN());  // NaN when not enough data
        }
    } else {
        dv2_line->set(0, std::numeric_limits<double>::quiet_NaN());  // NaN when not enough data
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
    if (datas.empty() || !datas[0] || !datas[0]->lines) {
        return;
    }
    
    auto data_lines = datas[0]->lines;
    auto dv2_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(dv2));
    if (!dv2_buffer || !data_lines) {
        return;
    }
    
    // Reset the buffer for fresh calculation
    dv2_buffer->reset();
    
    int data_size = end - start;  // Use the actual range, not end
    
    // LineBuffer starts with a NaN value after reset(), so we need to track if we've set the first real value
    bool first_value_set = false;
    
    // Calculate CHL values for all bars
    std::vector<double> all_chl_values;
    for (int i = start; i < end; ++i) {
        // DataSeries lines: 0=Open, 1=High, 2=Low, 3=Close, 4=Volume
        // Need to use ago indexing: 0 = current, -1 = previous, etc.
        // Since we're iterating forward, we need to calculate the ago index
        int ago_index = i - (end - 1);  // Convert absolute index to ago index
        double high = (*data_lines->getline(1))[ago_index];   // High
        double low = (*data_lines->getline(2))[ago_index];    // Low
        double close = (*data_lines->getline(3))[ago_index];  // Close
        
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
    
    
    
    // Calculate DV2 values chronologically (from oldest to newest)
    for (int i = 0; i < (end - start); ++i) {
        double dv2_value = std::numeric_limits<double>::quiet_NaN();
        
        int dvu_index = i - params.maperiod + 1;
        
        
        bool condition_met = (dvu_index >= 0 && dvu_index < static_cast<int>(all_dvu_values.size()) && 
                              dvu_index >= params.period - 1);
        
        if (condition_met) {
            
            
            // Get period data for percent rank
            std::vector<double> period_data;
            int pr_start = std::max(0, dvu_index - params.period + 1);
            for (int j = pr_start; j <= dvu_index; ++j) {
                period_data.push_back(all_dvu_values[j]);
            }
            
            
            // Calculate percent rank (Python: fsum(x < d[-1] for x in d) / len(d))
            // d[-1] is the last (most recent) value in the period
            double current_dvu = period_data.back();  // The last value in period_data
            int count_less = 0;
            for (double value : period_data) {
                if (value < current_dvu) {
                    count_less++;
                }
            }
            
            
            // Include the current value in the denominator
            double percent_rank = static_cast<double>(count_less) / period_data.size();
            dv2_value = percent_rank * 100.0;  // Convert to percentage
            
        } else {
            dv2_value = std::numeric_limits<double>::quiet_NaN();  // NaN when not enough data
        }
        
        // Set first value or append subsequent values
        if (!first_value_set) {
            dv2_buffer->set(0, dv2_value);
            first_value_set = true;
        } else {
            dv2_buffer->append(dv2_value);
        }
        
    }
    
    // Set the LineBuffer index to the end position for proper ago indexing
    if (dv2_buffer->size() > 0) {
        dv2_buffer->set_idx(dv2_buffer->size() - 1);
    }
}

} // namespace indicators
} // namespace backtrader