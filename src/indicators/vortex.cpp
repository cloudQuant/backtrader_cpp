#include "indicators/vortex.h"
#include "dataseries.h"
#include "linebuffer.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>

namespace backtrader {

// Vortex implementation
Vortex::Vortex() : Indicator() {
    setup_lines();
    _minperiod(params.period + 1); // Need previous values
}

// DataSeries constructors for disambiguation
Vortex::Vortex(std::shared_ptr<DataSeries> data_source) : Indicator() {
    setup_lines();
    _minperiod(params.period + 1); // Need previous values
    
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
}

Vortex::Vortex(std::shared_ptr<DataSeries> data_source, int period) : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1); // Need previous values
    
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
}

Vortex::Vortex(std::shared_ptr<LineSeries> data_source, int period) : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1); // Need previous values
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

void Vortex::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Vortex::prenext() {
    // Append NaN values for output lines before minimum period is reached
    auto vi_plus_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(vi_plus));
    auto vi_minus_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(vi_minus));
    
    if (vi_plus_line && vi_minus_line) {
        vi_plus_line->append(std::numeric_limits<double>::quiet_NaN());
        vi_minus_line->append(std::numeric_limits<double>::quiet_NaN());
    }
}

void Vortex::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);   // High line
    auto low_line = datas[0]->lines->getline(2);    // Low line
    auto close_line = datas[0]->lines->getline(4);  // Close line
    
    if (!high_line || !low_line || !close_line) {
        std::cout << "Vortex: Missing required lines" << std::endl;
        return;
    }
    
    // Check if we have enough data (need at least 2 bars for previous values)
    if (high_line->size() < 2 || low_line->size() < 2 || close_line->size() < 2) {
        // Not enough data yet, append NaN
        auto vi_plus_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(vi_plus));
        auto vi_minus_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(vi_minus));
        
        if (vi_plus_line && vi_minus_line) {
            vi_plus_line->append(std::numeric_limits<double>::quiet_NaN());
            vi_minus_line->append(std::numeric_limits<double>::quiet_NaN());
        }
        return;
    }
    
    // Current values
    double high0 = (*high_line)[0];
    double low0 = (*low_line)[0];
    double close0 = (*close_line)[0];
    
    // Previous values
    double high1 = (*high_line)[-1];
    double low1 = (*low_line)[-1];
    double close1 = (*close_line)[-1];
    
    // Calculate VM+ = |High[0] - Low[-1]|
    double vm_plus = std::abs(high0 - low1);
    vm_plus_values_.push_back(vm_plus);
    
    // Calculate VM- = |Low[0] - High[-1]|
    double vm_minus = std::abs(low0 - high1);
    vm_minus_values_.push_back(vm_minus);
    
    // Calculate True Range components
    double h0c1 = std::abs(high0 - close1);  // |High[0] - Close[-1]|
    double l0c1 = std::abs(low0 - close1);   // |Low[0] - Close[-1]|
    double h0l0 = std::abs(high0 - low0);    // |High[0] - Low[0]|
    
    // True Range = Max(H0-L0, H0-C1, L0-C1)
    double tr = std::max({h0l0, h0c1, l0c1});
    tr_values_.push_back(tr);
    
    // Keep only the period we need
    if (vm_plus_values_.size() > params.period) {
        vm_plus_values_.erase(vm_plus_values_.begin());
        vm_minus_values_.erase(vm_minus_values_.begin());
        tr_values_.erase(tr_values_.begin());
    }
    
    // Calculate Vortex Indicators
    auto vi_plus_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(vi_plus));
    auto vi_minus_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(vi_minus));
    
    if (vi_plus_line && vi_minus_line) {
        if (vm_plus_values_.size() >= params.period) {
            double sum_vm_plus = get_sum_vm_plus(params.period);
            double sum_vm_minus = get_sum_vm_minus(params.period);
            double sum_tr = get_sum_tr(params.period);
            
            if (sum_tr != 0.0) {
                vi_plus_line->append(sum_vm_plus / sum_tr);
                vi_minus_line->append(sum_vm_minus / sum_tr);
            } else {
                vi_plus_line->append(0.0);
                vi_minus_line->append(0.0);
            }
        } else {
            // Not enough data for the period yet, append NaN
            vi_plus_line->append(std::numeric_limits<double>::quiet_NaN());
            vi_minus_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
}

void Vortex::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    auto close_line = datas[0]->lines->getline(4);
    
    if (!high_line || !low_line || !close_line) return;
    
    // Build arrays for the entire range
    std::vector<double> all_vm_plus;
    std::vector<double> all_vm_minus;
    std::vector<double> all_tr;
    
    for (int i = start; i < end; ++i) {
        if (i > 0) { // Need previous values
            double high0 = (*high_line)[i];
            double low0 = (*low_line)[i];
            double close0 = (*close_line)[i];
            
            double high1 = (*high_line)[i - 1];
            double low1 = (*low_line)[i - 1];
            double close1 = (*close_line)[i - 1];
            
            // Calculate VM+ and VM-
            double vm_plus = std::abs(high0 - low1);
            double vm_minus = std::abs(low0 - high1);
            
            all_vm_plus.push_back(vm_plus);
            all_vm_minus.push_back(vm_minus);
            
            // Calculate True Range
            double h0c1 = std::abs(high0 - close1);
            double l0c1 = std::abs(low0 - close1);
            double h0l0 = std::abs(high0 - low0);
            
            double tr = std::max({h0l0, h0c1, l0c1});
            all_tr.push_back(tr);
        }
    }
    
    auto vi_plus_line = lines->getline(vi_plus);
    auto vi_minus_line = lines->getline(vi_minus);
    
    if (!vi_plus_line || !vi_minus_line) return;
    
    for (int i = start; i < end; ++i) {
        int idx = i - start - 1; // Adjust for needing previous values
        
        if (idx >= 0 && idx + 1 >= params.period) {
            // Calculate sums for the period
            double sum_vm_plus = 0.0;
            double sum_vm_minus = 0.0;
            double sum_tr = 0.0;
            
            for (int j = 0; j < params.period; ++j) {
                int data_idx = idx - j;
                if (data_idx >= 0 && data_idx < all_vm_plus.size()) {
                    sum_vm_plus += all_vm_plus[data_idx];
                    sum_vm_minus += all_vm_minus[data_idx];
                    sum_tr += all_tr[data_idx];
                }
            }
            
            if (sum_tr != 0.0) {
                vi_plus_line->set(i, sum_vm_plus / sum_tr);
                vi_minus_line->set(i, sum_vm_minus / sum_tr);
            } else {
                vi_plus_line->set(i, 0.0);
                vi_minus_line->set(i, 0.0);
            }
        } else {
            vi_plus_line->set(i, 0.0);
            vi_minus_line->set(i, 0.0);
        }
    }
}

double Vortex::get_sum_vm_plus(int period) {
    if (vm_plus_values_.size() < period) return 0.0;
    
    double sum = 0.0;
    int start_idx = std::max(0, static_cast<int>(vm_plus_values_.size()) - period);
    for (size_t i = start_idx; i < vm_plus_values_.size(); ++i) {
        sum += vm_plus_values_[i];
    }
    return sum;
}

double Vortex::get_sum_vm_minus(int period) {
    if (vm_minus_values_.size() < period) return 0.0;
    
    double sum = 0.0;
    int start_idx = std::max(0, static_cast<int>(vm_minus_values_.size()) - period);
    for (size_t i = start_idx; i < vm_minus_values_.size(); ++i) {
        sum += vm_minus_values_[i];
    }
    return sum;
}

double Vortex::get_sum_tr(int period) {
    if (tr_values_.size() < period) return 0.0;
    
    double sum = 0.0;
    int start_idx = std::max(0, static_cast<int>(tr_values_.size()) - period);
    for (size_t i = start_idx; i < tr_values_.size(); ++i) {
        sum += tr_values_[i];
    }
    return sum;
}

double Vortex::get(int ago) const {
    // Return VI+ by default (first line)
    return getVIPlus(ago);
}

int Vortex::getMinPeriod() const {
    return params.period + 1;
}

void Vortex::calculate() {
    if (datas.empty() || !datas[0] || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(DataSeries::High);
    auto low_line = datas[0]->lines->getline(DataSeries::Low);
    auto close_line = datas[0]->lines->getline(DataSeries::Close);
    
    if (!high_line || !low_line || !close_line) return;
    
    // Get LineBuffers for direct array access
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    
    if (!high_buffer || !low_buffer || !close_buffer) return;
    
    const auto& high_array = high_buffer->array();
    const auto& low_array = low_buffer->array();
    const auto& close_array = close_buffer->array();
    
    size_t data_size = high_array.size();
    if (data_size < 2) return; // Need at least 2 bars
    
    // Get output line buffers
    auto vi_plus_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(vi_plus));
    auto vi_minus_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(vi_minus));
    
    if (!vi_plus_buffer || !vi_minus_buffer) return;
    
    // Clear and initialize output buffers
    vi_plus_buffer->reset();
    vi_minus_buffer->reset();
    
    // Calculate all VM+, VM- and TR values
    std::vector<double> all_vm_plus(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> all_vm_minus(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> all_tr(data_size, std::numeric_limits<double>::quiet_NaN());
    
    for (size_t i = 1; i < data_size; ++i) {
        if (!std::isnan(high_array[i]) && !std::isnan(low_array[i]) && 
            !std::isnan(close_array[i]) && !std::isnan(high_array[i-1]) && 
            !std::isnan(low_array[i-1]) && !std::isnan(close_array[i-1])) {
            
            // VM+ = |High[i] - Low[i-1]|
            all_vm_plus[i] = std::abs(high_array[i] - low_array[i-1]);
            
            // VM- = |Low[i] - High[i-1]|
            all_vm_minus[i] = std::abs(low_array[i] - high_array[i-1]);
            
            // True Range = Max(High[i]-Low[i], |High[i]-Close[i-1]|, |Low[i]-Close[i-1]|)
            double h0l0 = high_array[i] - low_array[i];
            double h0c1 = std::abs(high_array[i] - close_array[i-1]);
            double l0c1 = std::abs(low_array[i] - close_array[i-1]);
            all_tr[i] = std::max({h0l0, h0c1, l0c1});
        }
    }
    
    // Calculate Vortex Indicators for each point
    for (size_t i = 0; i < data_size; ++i) {
        if (i == 0) {
            vi_plus_buffer->set(0, std::numeric_limits<double>::quiet_NaN());
            vi_minus_buffer->set(0, std::numeric_limits<double>::quiet_NaN());
        } else if (i < static_cast<size_t>(params.period)) {
            // Not enough data for full period
            vi_plus_buffer->append(std::numeric_limits<double>::quiet_NaN());
            vi_minus_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Calculate sums over the period (looking back params.period bars)
            double sum_vm_plus = 0.0;
            double sum_vm_minus = 0.0;
            double sum_tr = 0.0;
            int valid_count = 0;
            
            // Sum the most recent 'period' values including current bar
            for (int j = 0; j < params.period; ++j) {
                int idx = i - j;  // Go from current bar backwards
                if (idx >= 1 && idx < static_cast<int>(data_size) && !std::isnan(all_vm_plus[idx])) {
                    sum_vm_plus += all_vm_plus[idx];
                    sum_vm_minus += all_vm_minus[idx];
                    sum_tr += all_tr[idx];
                    valid_count++;
                }
            }
            
            if (valid_count == params.period && sum_tr != 0.0) {
                vi_plus_buffer->append(sum_vm_plus / sum_tr);
                vi_minus_buffer->append(sum_vm_minus / sum_tr);
            } else {
                vi_plus_buffer->append(std::numeric_limits<double>::quiet_NaN());
                vi_minus_buffer->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Set the buffer indices to the last valid position
    int last_valid_idx = static_cast<int>(data_size) - 1;
    vi_plus_buffer->set_idx(last_valid_idx);
    vi_minus_buffer->set_idx(last_valid_idx);
}

double Vortex::getVIPlus(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto vi_plus_line = lines->getline(vi_plus);
    if (!vi_plus_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Handle negative ago values for historical data access
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(vi_plus_line);
    if (buffer && ago < 0) {
        int idx = static_cast<int>(buffer->size()) - 1 + ago;
        if (idx >= 0 && idx < static_cast<int>(buffer->size())) {
            return buffer->array()[idx];
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*vi_plus_line)[ago];
}

double Vortex::getVIMinus(int ago) const {
    if (!lines || lines->size() < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto vi_minus_line = lines->getline(vi_minus);
    if (!vi_minus_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Handle negative ago values for historical data access
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(vi_minus_line);
    if (buffer && ago < 0) {
        int idx = static_cast<int>(buffer->size()) - 1 + ago;
        if (idx >= 0 && idx < static_cast<int>(buffer->size())) {
            return buffer->array()[idx];
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*vi_minus_line)[ago];
}

size_t Vortex::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto vi_plus_line = lines->getline(vi_plus);
    return vi_plus_line ? vi_plus_line->size() : 0;
}

} // namespace backtrader