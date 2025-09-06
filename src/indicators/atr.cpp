#include "indicators/atr.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace backtrader {
namespace indicators {

// TrueHigh implementation
TrueHigh::TrueHigh() : Indicator() {
    setup_lines();
    _minperiod(2); // Needs previous close
}

void TrueHigh::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void TrueHigh::prenext() {
    Indicator::prenext();
}

void TrueHigh::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1); // High
    auto close_line = datas[0]->lines->getline(4); // Close
    auto truehigh_line = lines->getline(truehigh);
    
    if (high_line && close_line && truehigh_line) {
        double current_high = (*high_line)[0];
        double prev_close = (*close_line)[-1];
        double true_high = std::max(current_high, prev_close);
        truehigh_line->set(0, true_high);
    }
}

void TrueHigh::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);
    auto close_line = datas[0]->lines->getline(4);
    auto truehigh_line = lines->getline(truehigh);
    
    if (!high_line || !close_line || !truehigh_line) return;
    
    for (int i = start; i < end; ++i) {
        // Convert array index to LineBuffer index
        int high_linebuffer_index = static_cast<int>(high_line->size()) - 1 - i;
        int close_linebuffer_index = static_cast<int>(close_line->size()) - 1 - i;
        int prev_close_linebuffer_index = (i > 0) ? static_cast<int>(close_line->size()) - 1 - (i-1) : close_linebuffer_index;
        double current_high = (*high_line)[high_linebuffer_index];
        double prev_close = (*close_line)[prev_close_linebuffer_index];
        double true_high = std::max(current_high, prev_close);
        truehigh_line->set(i, true_high);
    }
}

double TrueHigh::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto truehigh_line = lines->getline(0);
    if (!truehigh_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*truehigh_line)[ago];
}

// TrueLow implementation
TrueLow::TrueLow() : Indicator() {
    setup_lines();
    _minperiod(2); // Needs previous close
}

void TrueLow::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void TrueLow::prenext() {
    Indicator::prenext();
}

void TrueLow::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto low_line = datas[0]->lines->getline(2); // Low
    auto close_line = datas[0]->lines->getline(4); // Close
    auto truelow_line = lines->getline(truelow);
    
    if (low_line && close_line && truelow_line) {
        double current_low = (*low_line)[0];
        double prev_close = (*close_line)[-1];
        double true_low = std::min(current_low, prev_close);
        truelow_line->set(0, true_low);
    }
}

void TrueLow::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto low_line = datas[0]->lines->getline(2);
    auto close_line = datas[0]->lines->getline(4);
    auto truelow_line = lines->getline(truelow);
    
    if (!low_line || !close_line || !truelow_line) return;
    
    for (int i = start; i < end; ++i) {
        // Convert array index to LineBuffer index
        int low_linebuffer_index = static_cast<int>(low_line->size()) - 1 - i;
        int close_linebuffer_index = static_cast<int>(close_line->size()) - 1 - i;
        int prev_close_linebuffer_index = (i > 0) ? static_cast<int>(close_line->size()) - 1 - (i-1) : close_linebuffer_index;
        double current_low = (*low_line)[low_linebuffer_index];
        double prev_close = (*close_line)[prev_close_linebuffer_index];
        double true_low = std::min(current_low, prev_close);
        truelow_line->set(i, true_low);
    }
}

double TrueLow::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto truelow_line = lines->getline(0);
    if (!truelow_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*truelow_line)[ago];
}

// TrueRange implementation
TrueRange::TrueRange() : Indicator() {
    setup_lines();
    _minperiod(2); // Needs previous close
}

void TrueRange::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double TrueRange::calculate_true_range(double high, double low, double prev_close) const {
    double range1 = high - low;
    double range2 = std::abs(high - prev_close);
    double range3 = std::abs(prev_close - low);
    
    return std::max({range1, range2, range3});
}

void TrueRange::prenext() {
    Indicator::prenext();
}

void TrueRange::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1); // High
    auto low_line = datas[0]->lines->getline(2);  // Low
    auto close_line = datas[0]->lines->getline(4); // Close
    auto tr_line = lines->getline(tr);
    
    if (high_line && low_line && close_line && tr_line) {
        double current_high = (*high_line)[0];
        double current_low = (*low_line)[0];
        double prev_close = (*close_line)[-1];
        
        double tr_value = calculate_true_range(current_high, current_low, prev_close);
        tr_line->set(0, tr_value);
    }
}

void TrueRange::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(2);  // high
    auto low_line = datas[0]->lines->getline(3);   // low
    auto close_line = datas[0]->lines->getline(4); // close
    auto tr_line = lines->getline(tr);
    
    if (!high_line || !low_line || !close_line || !tr_line) return;
    
    for (int i = start; i < end; ++i) {
        // Convert array index to LineBuffer index
        int high_linebuffer_index = static_cast<int>(high_line->size()) - 1 - i;
        int low_linebuffer_index = static_cast<int>(low_line->size()) - 1 - i;
        int close_linebuffer_index = static_cast<int>(close_line->size()) - 1 - i;
        int prev_close_linebuffer_index = (i > 0) ? static_cast<int>(close_line->size()) - 1 - (i-1) : close_linebuffer_index;
        double current_high = (*high_line)[high_linebuffer_index];
        double current_low = (*low_line)[low_linebuffer_index];
        double prev_close = (*close_line)[prev_close_linebuffer_index];
        
        double tr_value = calculate_true_range(current_high, current_low, prev_close);
        tr_line->set(i, tr_value);
    }
}

double TrueRange::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto tr_line = lines->getline(0);
    if (!tr_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*tr_line)[ago];
}

// AverageTrueRange implementation
AverageTrueRange::AverageTrueRange() : Indicator(), prev_atr_(0.0), first_calculation_(true), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period + 1); // Need period + 1 for smoothed average
}

AverageTrueRange::AverageTrueRange(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), prev_atr_(0.0), first_calculation_(true), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1); // Need period + 1 for smoothed average
}

AverageTrueRange::AverageTrueRange(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), prev_atr_(0.0), first_calculation_(true), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1); // Need period + 1 for smoothed average
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

void AverageTrueRange::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double AverageTrueRange::calculate_true_range(double high, double low, double prev_close) const {
    double range1 = high - low;
    double range2 = std::abs(high - prev_close);
    double range3 = std::abs(prev_close - low);
    
    return std::max({range1, range2, range3});
}

double AverageTrueRange::calculate_smoothed_average(const std::vector<double>& values, int period) const {
    if (values.size() < static_cast<size_t>(period)) return 0.0;
    
    // First calculation: simple average of first 'period' values
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += values[i];
    }
    return sum / period;
}

void AverageTrueRange::prenext() {
    Indicator::prenext();
}

void AverageTrueRange::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(2);  // high
    auto low_line = datas[0]->lines->getline(3);   // low
    auto close_line = datas[0]->lines->getline(4); // close
    auto atr_line = lines->getline(atr);
    
    if (!high_line || !low_line || !close_line || !atr_line) return;
    
    // First, advance the line buffer to make room for new value
    atr_line->advance();
    
    double current_high = (*high_line)[0];
    double current_low = (*low_line)[0];
    double prev_close = (*close_line)[-1];
    
    double tr_value = calculate_true_range(current_high, current_low, prev_close);
    tr_history_.push_back(tr_value);
    
    // Keep only the necessary history
    if (tr_history_.size() > static_cast<size_t>(params.period * 2)) {
        tr_history_.erase(tr_history_.begin());
    }
    
    double atr_value;
    if (first_calculation_ && tr_history_.size() >= static_cast<size_t>(params.period)) {
        // First ATR calculation: simple average
        atr_value = calculate_smoothed_average(tr_history_, params.period);
        first_calculation_ = false;
    } else if (!first_calculation_) {
        // Subsequent calculations: Wilder's smoothing
        // ATR = ((n-1) * previous_ATR + current_TR) / n
        atr_value = ((params.period - 1) * prev_atr_ + tr_value) / params.period;
    } else {
        // Not enough data yet
        atr_value = tr_value;
    }
    
    prev_atr_ = atr_value;
    atr_line->set(0, atr_value);
}

void AverageTrueRange::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(2);  // high
    auto low_line = datas[0]->lines->getline(3);   // low
    auto close_line = datas[0]->lines->getline(4); // close
    auto atr_line_single = lines->getline(atr);
    
    if (!high_line || !low_line || !close_line || !atr_line_single) return;
    
    // Get LineBuffers for direct array access
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    auto atr_buffer = std::dynamic_pointer_cast<LineBuffer>(atr_line_single);
    
    if (!high_buffer || !low_buffer || !close_buffer || !atr_buffer) return;
    
    // Clear ATR buffer (don't use reset() which adds an initial NaN)
    atr_buffer->clear();
    
    const auto& high_array = high_buffer->array();
    const auto& low_array = low_buffer->array();
    const auto& close_array = close_buffer->array();
    
    int data_size = static_cast<int>(high_array.size());
    if (data_size == 0) return;
    
    
    // First value is NaN (no previous close)
    atr_buffer->append(std::numeric_limits<double>::quiet_NaN());
    
    // Calculate TR and ATR values
    std::vector<double> tr_values;  // Store TR values for initial ATR calculation
    double current_atr = 0.0;
    
    for (int i = 1; i < data_size && i < end; ++i) {
        // Calculate True Range
        double high_low = high_array[i] - low_array[i];
        double high_close = std::abs(high_array[i] - close_array[i-1]);
        double low_close = std::abs(low_array[i] - close_array[i-1]);
        double tr = std::max({high_low, high_close, low_close});
        
        // Store TR value
        tr_values.push_back(tr);
        
        if (tr_values.size() < static_cast<size_t>(params.period)) {
            // Still accumulating, output NaN
            atr_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else if (tr_values.size() == static_cast<size_t>(params.period)) {
            // We have exactly 'period' TR values
            // First ATR: simple average of these TR values
            double sum = 0.0;
            for (size_t j = tr_values.size() - params.period; j < tr_values.size(); ++j) {
                sum += tr_values[j];
            }
            current_atr = sum / params.period;
            atr_buffer->append(current_atr);
        } else {
            // Subsequent ATR: Wilder's smoothing
            current_atr = ((params.period - 1) * current_atr + tr) / params.period;
            atr_buffer->append(current_atr);
        }
    }
    
    // Set line buffer index to the last position
    if (atr_buffer->data_size() > 0) {
        atr_buffer->set_idx(atr_buffer->data_size() - 1);
    }
}

double AverageTrueRange::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto atr_line = lines->getline(0);
    if (!atr_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*atr_line)[ago];
}

void AverageTrueRange::calculate() {
    // Use data_source_ directly if datas is empty (common case for constructed ATR)
    std::shared_ptr<DataSeries> working_data = nullptr;
    
    if (!datas.empty() && datas[0]) {
        working_data = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    } else if (data_source_) {
        working_data = std::dynamic_pointer_cast<DataSeries>(data_source_);
    }
    
    if (!working_data || !working_data->lines || working_data->lines->size() < 5) {
        return;
    }
    
    // Temporarily populate datas for the rest of the method if needed
    if (datas.empty()) {
        datas.push_back(working_data);
    }
    
    // Get the high line to determine data size
    auto high_line = datas[0]->lines->getline(2);
    
    if (!high_line) {
            return;
    }
    
    // Get LineBuffer for size
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    int data_size = high_buffer ? high_buffer->array().size() : high_line->size();
    
    
    // Calculate ATR for the entire dataset using once() method
    once(0, data_size);
}

} // namespace indicators
} // namespace backtrader