#include "indicators/vortex.h"
#include <algorithm>
#include <cmath>

namespace backtrader {

// Vortex implementation
Vortex::Vortex() : Indicator() {
    setup_lines();
    _minperiod(params.period + 1); // Need previous values
}

void Vortex::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Vortex::prenext() {
    Indicator::prenext();
}

void Vortex::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);   // High line
    auto low_line = datas[0]->lines->getline(2);    // Low line
    auto close_line = datas[0]->lines->getline(3);  // Close line
    
    if (!high_line || !low_line || !close_line) return;
    
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
    auto vi_plus_line = lines->getline(vi_plus);
    auto vi_minus_line = lines->getline(vi_minus);
    
    if (vi_plus_line && vi_minus_line && vm_plus_values_.size() >= params.period) {
        double sum_vm_plus = get_sum_vm_plus(params.period);
        double sum_vm_minus = get_sum_vm_minus(params.period);
        double sum_tr = get_sum_tr(params.period);
        
        if (sum_tr != 0.0) {
            vi_plus_line->set(0, sum_vm_plus / sum_tr);
            vi_minus_line->set(0, sum_vm_minus / sum_tr);
        } else {
            vi_plus_line->set(0, 0.0);
            vi_minus_line->set(0, 0.0);
        }
    }
}

void Vortex::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    auto close_line = datas[0]->lines->getline(3);
    
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

} // namespace backtrader