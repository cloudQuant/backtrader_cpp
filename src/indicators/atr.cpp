#include "indicators/atr.h"
#include <algorithm>
#include <cmath>
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
    auto close_line = datas[0]->lines->getline(3); // Close
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
    auto close_line = datas[0]->lines->getline(3);
    auto truehigh_line = lines->getline(truehigh);
    
    if (!high_line || !close_line || !truehigh_line) return;
    
    for (int i = start; i < end; ++i) {
        double current_high = (*high_line)[i];
        double prev_close = (i > 0) ? (*close_line)[i-1] : (*close_line)[i];
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
    auto close_line = datas[0]->lines->getline(3); // Close
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
    auto close_line = datas[0]->lines->getline(3);
    auto truelow_line = lines->getline(truelow);
    
    if (!low_line || !close_line || !truelow_line) return;
    
    for (int i = start; i < end; ++i) {
        double current_low = (*low_line)[i];
        double prev_close = (i > 0) ? (*close_line)[i-1] : (*close_line)[i];
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
    auto close_line = datas[0]->lines->getline(3); // Close
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
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    auto close_line = datas[0]->lines->getline(3);
    auto tr_line = lines->getline(tr);
    
    if (!high_line || !low_line || !close_line || !tr_line) return;
    
    for (int i = start; i < end; ++i) {
        double current_high = (*high_line)[i];
        double current_low = (*low_line)[i];
        double prev_close = (i > 0) ? (*close_line)[i-1] : (*close_line)[i];
        
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

AverageTrueRange::AverageTrueRange(std::shared_ptr<LineRoot> data) : Indicator(), prev_atr_(0.0), first_calculation_(true), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period + 1); // Need period + 1 for smoothed average
    // This constructor is for test framework compatibility
    // The data parameter is ignored since ATR needs full OHLC data from datas[0]
}

AverageTrueRange::AverageTrueRange(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), prev_atr_(0.0), first_calculation_(true), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1); // Need period + 1 for smoothed average
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
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    auto close_line = datas[0]->lines->getline(3);
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
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    auto close_line = datas[0]->lines->getline(3);
    auto atr_line = lines->getline(atr);
    
    if (!high_line || !low_line || !close_line || !atr_line) return;
    
    std::vector<double> tr_values;
    
    // Calculate all TR values first
    for (int i = start; i < end; ++i) {
        double current_high = (*high_line)[i];
        double current_low = (*low_line)[i];
        double prev_close = (i > 0) ? (*close_line)[i-1] : (*close_line)[i];
        
        double tr_value = calculate_true_range(current_high, current_low, prev_close);
        tr_values.push_back(tr_value);
    }
    
    // Calculate ATR values
    for (int i = start; i < end; ++i) {
        int tr_index = i - start;
        
        if (tr_index < params.period - 1) {
            // Not enough data yet
            atr_line->set(i, tr_values[tr_index]);
        } else if (tr_index == params.period - 1) {
            // First ATR: simple average
            double sum = 0.0;
            for (int j = 0; j <= tr_index; ++j) {
                sum += tr_values[j];
            }
            atr_line->set(i, sum / params.period);
        } else {
            // Subsequent ATRs: Wilder's smoothing
            double prev_atr = (*atr_line)[i-1];
            double current_tr = tr_values[tr_index];
            double atr_value = ((params.period - 1) * prev_atr + current_tr) / params.period;
            atr_line->set(i, atr_value);
        }
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
    if (!data_source_ || !data_source_->lines || data_source_->lines->size() == 0) {
        return;
    }
    
    auto data_line = data_source_->lines->getline(0);
    
    if (!data_line) {
        return;
    }
    
    // Calculate ATR for the entire dataset using once() method
    once(0, data_line->size());
}

} // namespace indicators
} // namespace backtrader