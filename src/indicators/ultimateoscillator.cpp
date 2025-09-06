#include "indicators/ultimateoscillator.h"
#include "indicator_utils.h"
#include "dataseries.h"
#include "linebuffer.h"
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

// DataSeries constructors for disambiguation
UltimateOscillator::UltimateOscillator(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    
    // Need maximum of the three periods + 1 for true range calculation
    _minperiod(std::max({params.p1, params.p2, params.p3}) + 1);
    
    // Initialize circular buffers
    bp_values_.reserve(params.p3 + 10);
    tr_values_.reserve(params.p3 + 10);
    
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
}

UltimateOscillator::UltimateOscillator(std::shared_ptr<DataSeries> data_source, int p1, int p2, int p3) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.p1 = p1;
    params.p2 = p2;
    params.p3 = p3;
    setup_lines();
    
    // Need maximum of the three periods + 1 for true range calculation
    _minperiod(std::max({params.p1, params.p2, params.p3}) + 1);
    
    // Initialize circular buffers
    bp_values_.reserve(params.p3 + 10);
    tr_values_.reserve(params.p3 + 10);
    
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
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

size_t UltimateOscillator::size() const {
    // Override to use lines instead of lines_
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto line = lines->getline(0);
    if (!line) {
        return 0;
    }
    
    // Get the actual buffer to check its size
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    if (buffer) {
        // The buffer's size() method returns _idx + 1
        // After forwarding, the size should reflect the number of values
        return buffer->size();
    }
    
    return line->size();
}

int UltimateOscillator::getMinPeriod() const {
    return std::max({params.p1, params.p2, params.p3}) + 1;
}

void UltimateOscillator::calculate() {
    // Use once() for batch calculation if not done yet
    if (!batch_calculated_) {
        // Get data size
        std::shared_ptr<LineSeries> data_source;
        if (!datas.empty() && datas[0] && datas[0]->lines) {
            data_source = datas[0];
        } else if (data && data->lines) {
            data_source = data;
        } else if (data_source_ && data_source_->lines) {
            data_source = data_source_;
        } else {
            return;
        }
        
        auto close_line = data_source->lines->getline(4);  // Close
        if (!close_line) {
            return;
        }
        
        // For calculate(), we want to process all available data
        // Use data_size() to get the actual data size, not size() which depends on index
        auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
        int data_size = close_buffer ? close_buffer->data_size() : close_line->size();
        
        once(0, data_size);
        batch_calculated_ = true;
        
    }
}

void UltimateOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    
    // Connect LineCollection to IndicatorBase lines_ vector for size() method
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
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
    // Try to get data source from either datas[0] or data member
    std::shared_ptr<LineSeries> data_source;
    if (!datas.empty() && datas[0] && datas[0]->lines) {
        data_source = datas[0];
    } else if (data && data->lines) {
        data_source = data;
    } else if (data_source_ && data_source_->lines) {
        data_source = data_source_;
    } else {
        return;
    }
    
    auto high_line = data_source->lines->getline(2);   // High
    auto low_line = data_source->lines->getline(3);    // Low  
    auto close_line = data_source->lines->getline(4);  // Close
    auto uo_line = lines->getline(uo);
    
    if (!high_line || !low_line || !close_line || !uo_line) {
        return;
    }
    
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
    // Try to get data source from either datas[0] or data member
    std::shared_ptr<LineSeries> data_source;
    if (!datas.empty() && datas[0] && datas[0]->lines) {
        data_source = datas[0];
    } else if (data && data->lines) {
        data_source = data;
    } else if (data_source_ && data_source_->lines) {
        data_source = data_source_;
    } else {
        return;
    }
    
    auto high_line = data_source->lines->getline(2);   // High
    auto low_line = data_source->lines->getline(3);    // Low  
    auto close_line = data_source->lines->getline(4);  // Close
    auto uo_line = lines->getline(uo);
    
    if (!high_line || !low_line || !close_line || !uo_line) {
        return;
    }
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    auto uo_buffer = std::dynamic_pointer_cast<LineBuffer>(uo_line);
    
    if (!high_buffer || !low_buffer || !close_buffer || !uo_buffer) {
        return;
    }
    
    // Get the raw data pointers for direct access (bypasses index)
    const double* high_ptr = high_buffer->data_ptr();
    const double* low_ptr = low_buffer->data_ptr();
    const double* close_ptr = close_buffer->data_ptr();
    
    int data_size = high_buffer->data_size();  // Use data_size() not size()
    
    // Reset buffers for batch calculation
    bp_values_.clear();
    tr_values_.clear();
    bp_values_.reserve(data_size);
    tr_values_.reserve(data_size);
    
    // Reset output buffer
    uo_buffer->reset();
    
    // Process all data points
    // Find the first valid data point (skip initial NaN values)
    int start_idx = 0;
    for (int i = 0; i < data_size; ++i) {
        if (!std::isnan(close_ptr[i]) && !std::isnan(high_ptr[i]) && !std::isnan(low_ptr[i])) {
            start_idx = i;
            break;
        }
    }
    
    // Fill NaN for all positions before start_idx
    for (int i = 0; i < start_idx; ++i) {
        uo_buffer->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Process from first valid data point
    for (int i = start_idx; i < data_size; ++i) {
        double high = high_ptr[i];
        double low = low_ptr[i];
        double close = close_ptr[i];
        double prev_close = (i > start_idx && !std::isnan(close_ptr[i-1])) ? close_ptr[i-1] : close;
        
        // Skip if data is invalid
        if (std::isnan(high) || std::isnan(low) || std::isnan(close)) {
            uo_buffer->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        // Calculate buying pressure and true range
        double bp = calculate_buying_pressure(high, low, close, prev_close);
        double tr = calculate_true_range(high, low, prev_close);
        
        bp_values_.push_back(bp);
        tr_values_.push_back(tr);
        
        // Calculate Ultimate Oscillator if we have enough data
        if (bp_values_.size() >= static_cast<size_t>(params.p3)) {
            double sum_bp_p1 = 0.0, sum_tr_p1 = 0.0;
            double sum_bp_p2 = 0.0, sum_tr_p2 = 0.0;
            double sum_bp_p3 = 0.0, sum_tr_p3 = 0.0;
            
            // Calculate sums for each period looking back from current position
            int bp_size = bp_values_.size();
            for (int j = 0; j < params.p1 && j < bp_size; ++j) {
                sum_bp_p1 += bp_values_[bp_size - 1 - j];
                sum_tr_p1 += tr_values_[bp_size - 1 - j];
            }
            
            for (int j = 0; j < params.p2 && j < bp_size; ++j) {
                sum_bp_p2 += bp_values_[bp_size - 1 - j];
                sum_tr_p2 += tr_values_[bp_size - 1 - j];
            }
            
            for (int j = 0; j < params.p3 && j < bp_size; ++j) {
                sum_bp_p3 += bp_values_[bp_size - 1 - j];
                sum_tr_p3 += tr_values_[bp_size - 1 - j];
            }
            
            if (sum_tr_p1 > 0.0 && sum_tr_p2 > 0.0 && sum_tr_p3 > 0.0) {
                double av1 = sum_bp_p1 / sum_tr_p1;
                double av2 = sum_bp_p2 / sum_tr_p2;
                double av3 = sum_bp_p3 / sum_tr_p3;
                
                // Ultimate Oscillator = 100 * [(4*AV1) + (2*AV2) + AV3] / (4 + 2 + 1)
                double uo = 100.0 * (4.0 * av1 + 2.0 * av2 + av3) / 7.0;
                uo_buffer->append(uo);
            } else {
                uo_buffer->append(std::numeric_limits<double>::quiet_NaN());
            }
        } else {
            uo_buffer->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Set the buffer index to the last appended element
    if (uo_buffer->size() > 0) {
        int last_idx = uo_buffer->size() - 1;
        uo_buffer->set_idx(last_idx, true);
    }
}

} // namespace indicators
} // namespace backtrader