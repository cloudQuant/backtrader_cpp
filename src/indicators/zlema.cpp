#include "indicators/zlema.h"
#include "linebuffer.h"
#include <limits>
#include <cmath>
#include <algorithm>

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
}

ZeroLagExponentialMovingAverage::ZeroLagExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate lag
    lag_ = (params.period - 1) / 2;
    
    // Set minimum period needed
    _minperiod(params.period + lag_);
    
    // Set data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

ZeroLagExponentialMovingAverage::ZeroLagExponentialMovingAverage(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Calculate lag
    lag_ = (params.period - 1) / 2;
    
    // Set minimum period needed
    _minperiod(params.period + lag_);
    
    // Set data source
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        data_source_ = lineseries;
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
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

size_t ZeroLagExponentialMovingAverage::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto zlema_line = lines->getline(zlema);
    return zlema_line ? zlema_line->size() : 0;
}

void ZeroLagExponentialMovingAverage::calculate() {
    if (!datas.empty() && datas[0] && datas[0]->lines) {
        // Get the appropriate data line
        // For DataSeries, use close price (index 4), for LineSeries use first line (index 0)
        int line_idx = 0;
        if (datas[0]->lines->size() > 4) {
            // This is likely a DataSeries with OHLCV data
            line_idx = 4; // Close price
        }
        
        auto data_line = datas[0]->lines->getline(line_idx);
        if (data_line) {
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (data_buffer) {
                const auto& data_array = data_buffer->array();
                if (data_array.size() > 0) {
                    // Call once to perform the calculation
                    once(0, data_array.size());
                }
            }
        }
    }
}

void ZeroLagExponentialMovingAverage::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void ZeroLagExponentialMovingAverage::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get the appropriate data line
    // For DataSeries, use close price (index 4), for LineSeries use first line (index 0)
    int line_idx = 0;
    if (datas[0]->lines->size() > 4) {
        // This is likely a DataSeries with OHLCV data
        line_idx = 4; // Close price
    }
    
    auto data_line = datas[0]->lines->getline(line_idx);
    auto zlema_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(zlema));
    
    if (!data_line || !zlema_line) return;
    
    // Get current bar index
    size_t current_size = zlema_line->size();
    
    // Check if we have enough data
    if (data_line->size() < lag_ + 1) {
        zlema_line->append(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate zero-lag data: 2 * data - data(-lag)
    double current_data = (*data_line)[0];
    double lag_data = (*data_line)[-lag_];
    double zl_data = 2.0 * current_data - lag_data;
    
    // Calculate EMA of zero-lag data
    double alpha = 2.0 / (params.period + 1.0);
    
    if (current_size < params.period) {
        // Not enough data for EMA yet, just store the value
        zlema_line->append(zl_data);
    } else if (current_size == params.period) {
        // Calculate initial SMA as seed for EMA
        double sum = zl_data;  // Include current value
        for (int i = 1; i < params.period; ++i) {
            double prev_current = (*data_line)[-i];
            double prev_lag = (*data_line)[-(i + lag_)];
            double prev_zl = 2.0 * prev_current - prev_lag;
            sum += prev_zl;
        }
        double sma_seed = sum / params.period;
        zlema_line->append(sma_seed);
    } else {
        // Calculate EMA
        double prev_ema = zlema_line->array()[current_size - 1];
        double new_ema = alpha * zl_data + (1.0 - alpha) * prev_ema;
        zlema_line->append(new_ema);
    }
}

void ZeroLagExponentialMovingAverage::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get the appropriate data line
    // For DataSeries, use close price (index 4), for LineSeries use first line (index 0)
    int line_idx = 0;
    if (datas[0]->lines->size() > 4) {
        // This is likely a DataSeries with OHLCV data
        line_idx = 4; // Close price
    }
    
    auto data_line = datas[0]->lines->getline(line_idx);
    auto zlema_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(zlema));
    
    if (!data_line || !zlema_buffer) return;
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    const auto& data_array = data_buffer->array();
    
    // Clear and resize output buffer
    zlema_buffer->reset();
    
    // Calculate zero-lag data for all bars
    std::vector<double> zl_values;
    zl_values.reserve(end);
    
    for (int i = 0; i < end; ++i) {
        if (i < lag_) {
            zl_values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else {
            double current_data = data_array[i];
            double lag_data = data_array[i - lag_];
            if (!std::isnan(current_data) && !std::isnan(lag_data)) {
                double zl_data = 2.0 * current_data - lag_data;
                zl_values.push_back(zl_data);
            } else {
                zl_values.push_back(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Calculate EMA of zero-lag data
    double alpha = 2.0 / (params.period + 1.0);
    double ema_value = std::numeric_limits<double>::quiet_NaN();
    
    for (int i = 0; i < end; ++i) {
        if (i < lag_) {
            // Not enough data for zero-lag calculation
            zlema_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else if (i < params.period + lag_ - 1) {
            // Not enough data for full EMA period
            zlema_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else if (i == params.period + lag_ - 1) {
            // Calculate initial SMA as seed for EMA
            double sum = 0.0;
            int valid_count = 0;
            for (int j = lag_; j <= i; ++j) {
                if (!std::isnan(zl_values[j])) {
                    sum += zl_values[j];
                    valid_count++;
                }
            }
            if (valid_count > 0) {
                ema_value = sum / valid_count;
                zlema_buffer->append(ema_value);
            } else {
                zlema_buffer->append(std::numeric_limits<double>::quiet_NaN());
            }
        } else {
            // Calculate EMA
            if (!std::isnan(zl_values[i]) && !std::isnan(ema_value)) {
                ema_value = alpha * zl_values[i] + (1.0 - alpha) * ema_value;
                zlema_buffer->append(ema_value);
            } else {
                zlema_buffer->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Set the buffer index to the last valid position
    zlema_buffer->set_idx(end - 1);
}

} // namespace indicators
} // namespace backtrader