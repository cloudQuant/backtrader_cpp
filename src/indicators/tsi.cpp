#include "indicators/tsi.h"
#include <cmath>

namespace backtrader {

// TrueStrengthIndicator implementation
TrueStrengthIndicator::TrueStrengthIndicator() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // TSI needs pchange + period1 + period2 - 1 for full calculation
    _minperiod(params.pchange + params.period1 + params.period2 - 1);
}

TrueStrengthIndicator::TrueStrengthIndicator(std::shared_ptr<LineSeries> data_source, int period1, int period2) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period1 = period1;
    params.period2 = period2;
    setup_lines();
    
    // TSI needs pchange + period1 + period2 - 1 for full calculation
    _minperiod(params.pchange + params.period1 + params.period2 - 1);
}

TrueStrengthIndicator::TrueStrengthIndicator(std::shared_ptr<LineRoot> data, int period1, int period2) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period1 = period1;
    params.period2 = period2;
    setup_lines();
    
    // Convert LineRoot to LineSeries if possible
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data);
    if (lineseries) {
        data_source_ = lineseries;
    }
    
    // TSI needs pchange + period1 + period2 - 1 for full calculation
    _minperiod(params.pchange + params.period1 + params.period2 - 1);
}

void TrueStrengthIndicator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void TrueStrengthIndicator::prenext() {
    // Standard indicator prenext implementation
    Indicator::prenext();
}

void TrueStrengthIndicator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line) return;
    
    // Calculate price change
    double current_price = (*data_line)[0];
    double previous_price = (*data_line)[-params.pchange];
    double price_change = current_price - previous_price;
    double abs_price_change = std::abs(price_change);
    
    // Store price changes for calculation
    price_changes_.push_back(price_change);
    abs_price_changes_.push_back(abs_price_change);
    
    // Simple TSI calculation - direct EMA calculation without sub-indicators
    if (price_changes_.size() >= static_cast<size_t>(params.period1 + params.period2)) {
        // Calculate first level smoothing
        double pc_ema1 = 0.0, apc_ema1 = 0.0;
        double alpha1 = 2.0 / (1.0 + params.period1);
        
        for (int i = std::max(0, static_cast<int>(price_changes_.size()) - params.period1); i < static_cast<int>(price_changes_.size()); ++i) {
            pc_ema1 = alpha1 * price_changes_[i] + (1.0 - alpha1) * pc_ema1;
            apc_ema1 = alpha1 * abs_price_changes_[i] + (1.0 - alpha1) * apc_ema1;
        }
        
        // Calculate second level smoothing
        double pc_ema2 = pc_ema1;
        double apc_ema2 = apc_ema1;
        double alpha2 = 2.0 / (1.0 + params.period2);
        
        for (int j = 0; j < params.period2; ++j) {
            pc_ema2 = alpha2 * pc_ema1 + (1.0 - alpha2) * pc_ema2;
            apc_ema2 = alpha2 * apc_ema1 + (1.0 - alpha2) * apc_ema2;
        }
        
        // Calculate TSI
        auto tsi_line = lines->getline(tsi);
        if (tsi_line && apc_ema2 != 0.0) {
            tsi_line->set(0, 100.0 * (pc_ema2 / apc_ema2));
        } else if (tsi_line) {
            tsi_line->set(0, 0.0);
        }
    }
}

void TrueStrengthIndicator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line) return;
    
    // Simplified once implementation using next()
    for (int i = start; i < end; ++i) {
        // Set current position for the next() call
        next();
    }
}

double TrueStrengthIndicator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return 0.0;
    }
    
    auto tsi_line = lines->getline(tsi);
    if (!tsi_line) {
        return 0.0;
    }
    
    return (*tsi_line)[ago];
}

int TrueStrengthIndicator::getMinPeriod() const {
    return params.pchange + params.period1 + params.period2 - 1;
}

void TrueStrengthIndicator::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

} // namespace backtrader