#include "indicators/ultimateoscillator.h"
#include <algorithm>
#include <numeric>
#include <limits>

namespace backtrader {
namespace indicators {

// UltimateOscillator implementation
UltimateOscillator::UltimateOscillator() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Need maximum of the three periods + 1 for true range calculation
    _minperiod(std::max({params.p1, params.p2, params.p3}) + 1);
    
    // Initialize circular buffers
    bp_values_.reserve(params.p3 + 10);
    tr_values_.reserve(params.p3 + 10);
}

UltimateOscillator::UltimateOscillator(std::shared_ptr<LineSeries> data_source, int p1, int p2, int p3) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.p1 = p1;
    params.p2 = p2;
    params.p3 = p3;
    setup_lines();
    
    // Need maximum of the three periods + 1 for true range calculation
    _minperiod(std::max({params.p1, params.p2, params.p3}) + 1);
    
    // Initialize circular buffers
    bp_values_.reserve(params.p3 + 10);
    tr_values_.reserve(params.p3 + 10);
}

double UltimateOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(uo);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int UltimateOscillator::getMinPeriod() const {
    return std::max({params.p1, params.p2, params.p3}) + 1;
}

void UltimateOscillator::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void UltimateOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double UltimateOscillator::calculate_buying_pressure(double high, double low, double close, double prev_close) {
    // Buying Pressure = Close - True Low
    double true_low = std::min(low, prev_close);
    return close - true_low;
}

double UltimateOscillator::calculate_true_range(double high, double low, double prev_close) {
    // True Range = max(High - Low, |High - Previous Close|, |Low - Previous Close|)
    double hl = high - low;
    double hpc = std::abs(high - prev_close);
    double lpc = std::abs(low - prev_close);
    return std::max({hl, hpc, lpc});
}

double UltimateOscillator::get_sum_bp(int period) {
    if (bp_values_.size() < static_cast<size_t>(period)) {
        return 0.0;
    }
    
    double sum = 0.0;
    int start = std::max(0, static_cast<int>(bp_values_.size()) - period);
    for (int i = start; i < static_cast<int>(bp_values_.size()); ++i) {
        sum += bp_values_[i];
    }
    return sum;
}

double UltimateOscillator::get_sum_tr(int period) {
    if (tr_values_.size() < static_cast<size_t>(period)) {
        return 0.0;
    }
    
    double sum = 0.0;
    int start = std::max(0, static_cast<int>(tr_values_.size()) - period);
    for (int i = start; i < static_cast<int>(tr_values_.size()); ++i) {
        sum += tr_values_[i];
    }
    return sum;
}

void UltimateOscillator::prenext() {
    Indicator::prenext();
}

void UltimateOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);   // High
    auto low_line = datas[0]->lines->getline(2);    // Low  
    auto close_line = datas[0]->lines->getline(3);  // Close
    auto uo_line = lines->getline(uo);
    
    if (!high_line || !low_line || !close_line || !uo_line) return;
    
    double high = (*high_line)[0];
    double low = (*low_line)[0];
    double close = (*close_line)[0];
    double prev_close = (close_line->size() > 1) ? (*close_line)[-1] : close;
    
    // Calculate buying pressure and true range
    double bp = calculate_buying_pressure(high, low, close, prev_close);
    double tr = calculate_true_range(high, low, prev_close);
    
    // Store in circular buffers
    bp_values_.push_back(bp);
    tr_values_.push_back(tr);
    
    // Keep only what we need
    if (bp_values_.size() > static_cast<size_t>(params.p3 + 10)) {
        bp_values_.erase(bp_values_.begin());
    }
    if (tr_values_.size() > static_cast<size_t>(params.p3 + 10)) {
        tr_values_.erase(tr_values_.begin());
    }
    
    // Calculate Ultimate Oscillator if we have enough data
    if (bp_values_.size() >= static_cast<size_t>(params.p3) && 
        tr_values_.size() >= static_cast<size_t>(params.p3)) {
        
        double sum_bp_p1 = get_sum_bp(params.p1);
        double sum_tr_p1 = get_sum_tr(params.p1);
        double sum_bp_p2 = get_sum_bp(params.p2);
        double sum_tr_p2 = get_sum_tr(params.p2);
        double sum_bp_p3 = get_sum_bp(params.p3);
        double sum_tr_p3 = get_sum_tr(params.p3);
        
        if (sum_tr_p1 > 0.0 && sum_tr_p2 > 0.0 && sum_tr_p3 > 0.0) {
            double av1 = sum_bp_p1 / sum_tr_p1;
            double av2 = sum_bp_p2 / sum_tr_p2;
            double av3 = sum_bp_p3 / sum_tr_p3;
            
            // Ultimate Oscillator = 100 * [(4*AV1) + (2*AV2) + AV3] / (4 + 2 + 1)
            double uo = 100.0 * (4.0 * av1 + 2.0 * av2 + av3) / 7.0;
            
            uo_line->set(0, uo);
        } else {
            uo_line->set(0, std::numeric_limits<double>::quiet_NaN());
        }
    } else {
        uo_line->set(0, std::numeric_limits<double>::quiet_NaN());
    }
}

void UltimateOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);   // High
    auto low_line = datas[0]->lines->getline(2);    // Low  
    auto close_line = datas[0]->lines->getline(3);  // Close
    auto uo_line = lines->getline(uo);
    
    if (!high_line || !low_line || !close_line || !uo_line) return;
    
    // Reset buffers for batch calculation
    bp_values_.clear();
    tr_values_.clear();
    
    for (int i = 0; i < end; ++i) {
        double high = (*high_line)[i];
        double low = (*low_line)[i];
        double close = (*close_line)[i];
        double prev_close = (i > 0) ? (*close_line)[i-1] : close;
        
        // Calculate buying pressure and true range
        double bp = calculate_buying_pressure(high, low, close, prev_close);
        double tr = calculate_true_range(high, low, prev_close);
        
        bp_values_.push_back(bp);
        tr_values_.push_back(tr);
        
        // Calculate Ultimate Oscillator if we have enough data and are in calculation range
        if (i >= start && i >= params.p3 && bp_values_.size() >= static_cast<size_t>(params.p3)) {
            double sum_bp_p1 = 0.0, sum_tr_p1 = 0.0;
            double sum_bp_p2 = 0.0, sum_tr_p2 = 0.0;
            double sum_bp_p3 = 0.0, sum_tr_p3 = 0.0;
            
            // Calculate sums for each period
            for (int j = 0; j < params.p1; ++j) {
                if (i - j >= 0) {
                    sum_bp_p1 += bp_values_[i - j];
                    sum_tr_p1 += tr_values_[i - j];
                }
            }
            
            for (int j = 0; j < params.p2; ++j) {
                if (i - j >= 0) {
                    sum_bp_p2 += bp_values_[i - j];
                    sum_tr_p2 += tr_values_[i - j];
                }
            }
            
            for (int j = 0; j < params.p3; ++j) {
                if (i - j >= 0) {
                    sum_bp_p3 += bp_values_[i - j];
                    sum_tr_p3 += tr_values_[i - j];
                }
            }
            
            if (sum_tr_p1 > 0.0 && sum_tr_p2 > 0.0 && sum_tr_p3 > 0.0) {
                double av1 = sum_bp_p1 / sum_tr_p1;
                double av2 = sum_bp_p2 / sum_tr_p2;
                double av3 = sum_bp_p3 / sum_tr_p3;
                
                // Ultimate Oscillator = 100 * [(4*AV1) + (2*AV2) + AV3] / (4 + 2 + 1)
                double uo = 100.0 * (4.0 * av1 + 2.0 * av2 + av3) / 7.0;
                
                uo_line->set(i, uo);
            } else {
                uo_line->set(i, std::numeric_limits<double>::quiet_NaN());
            }
        } else {
            uo_line->set(i, std::numeric_limits<double>::quiet_NaN());
        }
    }
}

} // namespace indicators
} // namespace backtrader