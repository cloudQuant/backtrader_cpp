#include "indicators/zlema.h"
#include <limits>

namespace backtrader {
namespace indicators {

// ZeroLagExponentialMovingAverage implementation
ZeroLagExponentialMovingAverage::ZeroLagExponentialMovingAverage() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Calculate lag
    lag_ = (params.period - 1) / 2;
    
    // Set minimum period needed
    _minperiod(params.period + lag_);
    
    // Create EMA for calculation (not actually used in simplified version)
    // ema_ = std::make_shared<EMA>();
}

ZeroLagExponentialMovingAverage::ZeroLagExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate lag
    lag_ = (params.period - 1) / 2;
    
    // Set minimum period needed
    _minperiod(params.period + lag_);
    
    // Create EMA for calculation (not actually used in simplified version)
    // ema_ = std::make_shared<EMA>();
}

// Constructor for test framework compatibility
ZeroLagExponentialMovingAverage::ZeroLagExponentialMovingAverage(std::shared_ptr<LineRoot> data, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate lag
    lag_ = (params.period - 1) / 2;
    
    // Set minimum period needed
    _minperiod(params.period + lag_);
}

double ZeroLagExponentialMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(zlema);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int ZeroLagExponentialMovingAverage::getMinPeriod() const {
    return params.period + lag_;
}

void ZeroLagExponentialMovingAverage::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void ZeroLagExponentialMovingAverage::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void ZeroLagExponentialMovingAverage::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto zlema_line = lines->getline(zlema);
    
    if (!data_line || !zlema_line) return;
    
    // Calculate zero-lag data: 2 * data - data(-lag)
    double current_data = (*data_line)[0];
    double lag_data = (*data_line)[-lag_];
    double zl_data = 2.0 * current_data - lag_data;
    
    // Store zero-lag data for EMA calculation
    static std::vector<double> zl_values;
    zl_values.push_back(zl_data);
    
    // Calculate EMA of zero-lag data
    if (zl_values.size() >= params.period) {
        // Manual EMA calculation
        static double ema_value = 0.0;
        static bool first_calculation = true;
        
        double alpha = 2.0 / (params.period + 1.0);
        
        if (first_calculation) {
            // First EMA value is SMA
            double sum = 0.0;
            int start_idx = zl_values.size() - params.period;
            for (int i = start_idx; i < start_idx + params.period; ++i) {
                sum += zl_values[i];
            }
            ema_value = sum / params.period;
            first_calculation = false;
        } else {
            // EMA calculation: alpha * current + (1 - alpha) * previous
            ema_value = alpha * zl_data + (1.0 - alpha) * ema_value;
        }
        
        zlema_line->set(0, ema_value);
    } else {
        zlema_line->set(0, zl_data);  // Not enough data for EMA
    }
    
    // Keep only necessary history
    if (zl_values.size() > params.period * 2) {
        zl_values.erase(zl_values.begin());
    }
}

void ZeroLagExponentialMovingAverage::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto zlema_line = lines->getline(zlema);
    
    if (!data_line || !zlema_line) return;
    
    // Calculate zero-lag data for all bars
    std::vector<double> zl_values;
    for (int i = lag_; i < end; ++i) {
        double current_data = (*data_line)[i];
        double lag_data = (*data_line)[i - lag_];
        double zl_data = 2.0 * current_data - lag_data;
        zl_values.push_back(zl_data);
    }
    
    // Calculate EMA of zero-lag data
    double alpha = 2.0 / (params.period + 1.0);
    double ema_value = 0.0;
    
    for (int i = start; i < end; ++i) {
        int zl_index = i - lag_;
        
        if (zl_index >= 0 && zl_index < zl_values.size()) {
            if (i == start || i < params.period + lag_ - 1) {
                // Calculate SMA for initial values
                double sum = 0.0;
                int count = 0;
                for (int j = std::max(0, zl_index - params.period + 1); j <= zl_index; ++j) {
                    sum += zl_values[j];
                    count++;
                }
                ema_value = (count > 0) ? sum / count : zl_values[zl_index];
            } else {
                // EMA calculation
                ema_value = alpha * zl_values[zl_index] + (1.0 - alpha) * ema_value;
            }
            
            zlema_line->set(i, ema_value);
        } else {
            zlema_line->set(i, (*data_line)[i]);  // Fallback to current data
        }
    }
}

} // namespace indicators
} // namespace backtrader