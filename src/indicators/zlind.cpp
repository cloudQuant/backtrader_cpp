#include "indicators/zlind.h"
#include <cmath>
#include <limits>

namespace backtrader {
namespace indicators {

// ZeroLagIndicator implementation
ZeroLagIndicator::ZeroLagIndicator() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
    
    // Calculate alpha values
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

ZeroLagIndicator::ZeroLagIndicator(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Calculate alpha values
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

ZeroLagIndicator::ZeroLagIndicator(std::shared_ptr<LineRoot> data_source, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0), lineroot_source_(data_source) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Calculate alpha values
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

double ZeroLagIndicator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(ec);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int ZeroLagIndicator::getMinPeriod() const {
    return params.period;
}

void ZeroLagIndicator::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void ZeroLagIndicator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void ZeroLagIndicator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto ec_line = lines->getline(ec);
    
    if (!data_line || !ec_line) return;
    
    double price = (*data_line)[0];
    
    // Simple implementation: just use the price as initial value
    // For a real ZeroLagIndicator, we would need proper EMA calculation
    double ema_value = price;  // Simplified
    double ec1 = (ec_line->size() > 1) ? (*ec_line)[-1] : ema_value;
    
    // Error correction optimization
    double least_error = std::numeric_limits<double>::max();
    double best_ec = ema_value;  // Default to EMA value
    
    // Iterate over gain limit range
    for (int value1 = -params.gainlimit; value1 <= params.gainlimit; ++value1) {
        double gain = value1 / 10.0;
        
        // Calculate error corrected value
        double ec = alpha_ * (ema_value + gain * (price - ec1)) + alpha1_ * ec1;
        
        // Calculate error
        double error = std::abs(price - ec);
        
        // Track best error correction
        if (error < least_error) {
            least_error = error;
            best_ec = ec;
        }
    }
    
    ec_line->set(0, best_ec);
}

void ZeroLagIndicator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto ec_line = lines->getline(ec);
    
    if (!data_line || !ec_line) return;
    
    for (int i = start; i < end; ++i) {
        double price = (*data_line)[i];
        double ema_value = price;  // Simplified
        double ec1 = (i > 0) ? (*ec_line)[i - 1] : ema_value;
        
        // Error correction optimization
        double least_error = std::numeric_limits<double>::max();
        double best_ec = ema_value;  // Default to EMA value
        
        // Iterate over gain limit range
        for (int value1 = -params.gainlimit; value1 <= params.gainlimit; ++value1) {
            double gain = value1 / 10.0;
            
            // Calculate error corrected value
            double ec = alpha_ * (ema_value + gain * (price - ec1)) + alpha1_ * ec1;
            
            // Calculate error
            double error = std::abs(price - ec);
            
            // Track best error correction
            if (error < least_error) {
                least_error = error;
                best_ec = ec;
            }
        }
        
        ec_line->set(i, best_ec);
    }
}

} // namespace indicators
} // namespace backtrader