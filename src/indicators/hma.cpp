#include "indicators/hma.h"
#include <cmath>
#include <limits>

namespace backtrader {
namespace indicators {

// HullMovingAverage implementation
HullMovingAverage::HullMovingAverage() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
}

HullMovingAverage::HullMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
}

double HullMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(Lines::hma);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int HullMovingAverage::getMinPeriod() const {
    return params.period;
}

void HullMovingAverage::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        // This is a simplified version - full implementation would require
        // proper WMA calculations on LineSeries data
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void HullMovingAverage::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void HullMovingAverage::prenext() {
    Indicator::prenext();
}

void HullMovingAverage::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    auto hma_line = lines->getline(Lines::hma);
    
    if (!hma_line) return;
    
    // Get current price
    double price = (*data_lines->getline(0))[0];  // Close price
    
    // Store prices for calculation
    prices_.push_back(price);
    
    // Simple HMA approximation - use a weighted average
    if (prices_.size() >= params.period) {
        // Calculate simple moving average as placeholder
        double sum = 0.0;
        int start_idx = std::max(0, static_cast<int>(prices_.size()) - params.period);
        for (int i = start_idx; i < prices_.size(); ++i) {
            sum += prices_[i];
        }
        double avg = sum / params.period;
        hma_line->set(0, avg);
    } else {
        hma_line->set(0, price);
    }
    
    // Keep only necessary history
    if (prices_.size() > params.period) {
        prices_.erase(prices_.begin());
    }
}

void HullMovingAverage::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    auto hma_line = lines->getline(Lines::hma);
    
    if (!hma_line) return;
    
    // Simple HMA approximation for batch calculation
    for (int i = start; i < end; ++i) {
        double price = (*data_lines->getline(0))[i];  // Close price
        
        if (i >= params.period - 1) {
            // Calculate simple moving average as placeholder
            double sum = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum += (*data_lines->getline(0))[i - j];
            }
            double avg = sum / params.period;
            hma_line->set(i, avg);
        } else {
            hma_line->set(i, price);
        }
    }
}

} // namespace indicators
} // namespace backtrader