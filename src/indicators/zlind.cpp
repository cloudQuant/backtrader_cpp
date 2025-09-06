#include "indicators/zlind.h"
#include "indicator_utils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace backtrader {
namespace indicators {

// ZeroLagIndicator implementation
ZeroLagIndicator::ZeroLagIndicator() : Indicator(), data_source_(nullptr), current_index_(0), ema_initialized_(false) {
    setup_lines();
    _minperiod(params.period);
    
    // Calculate alpha values
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    ema_value_ = 0.0;
}

ZeroLagIndicator::ZeroLagIndicator(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0), ema_initialized_(false) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Calculate alpha values
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    ema_value_ = 0.0;
    
    // Set up data connection - IMPORTANT!
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
    }
}

ZeroLagIndicator::ZeroLagIndicator(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0), ema_initialized_(false) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Calculate alpha values
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
    ema_value_ = 0.0;
    
    // Set up data connection
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
        data_source_ = lineseries;
    }
}

double ZeroLagIndicator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(ec);
    if (!line || line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use LineBuffer's built-in Python-style indexing
    // ago=0 means the last value, negative ago goes back in history
    return (*line)[ago];
}

int ZeroLagIndicator::getMinPeriod() const {
    return params.period;
}

void ZeroLagIndicator::calculate() {
    // Get data size
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        data_size = utils::getDataSize(datas[0]);
    } else if (data_source_) {
        data_size = utils::getDataSize(data_source_);
    }
    
    if (data_size > 0) {
        // Use once() method for batch processing all data points
        once(0, data_size);
    }
}

size_t ZeroLagIndicator::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto ec_line = lines->getline(ec);
    return ec_line ? ec_line->size() : 0;
}

void ZeroLagIndicator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("ec", 0);
    }
}

void ZeroLagIndicator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Try to get data line - check number of lines first to avoid exception
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() > 4) {
        // DataSeries with multiple lines, use Close (index 4)
        data_line = datas[0]->lines->getline(4);
    } else if (datas[0]->lines->size() > 0) {
        // LineSeries with single line, use index 0
        data_line = datas[0]->lines->getline(0);
    } else {
        return;  // No lines available
    }
    auto ec_line = lines->getline(ec);
    
    if (!data_line || !ec_line) return;
    
    double price = (*data_line)[0];
    
    // Check for invalid price data
    if (std::isnan(price) || !std::isfinite(price)) {
        return;  // Skip this calculation if price is invalid
    }
    
    // Initialize EMA with first price if not initialized
    if (!ema_initialized_) {
        ema_value_ = price;
        ema_initialized_ = true;
    } else {
        // Update EMA: ema = alpha * price + alpha1 * previous_ema
        ema_value_ = alpha_ * price + alpha1_ * ema_value_;
    }
    
    // Get previous EC value for error correction - use get() method for safer access
    double ec1 = ema_value_;  // Default to current EMA
    if (ec_line->size() > 1) {
        // Try to get previous value safely
        try {
            ec1 = ec_line->get(-1);
            if (std::isnan(ec1) || !std::isfinite(ec1)) {
                ec1 = ema_value_;  // Fallback to EMA if previous value is invalid
            }
        } catch (...) {
            ec1 = ema_value_;  // Fallback on any access error
        }
    }
    
    // Error correction optimization
    double least_error = std::numeric_limits<double>::max();
    double best_ec = ema_value_;  // Default to EMA value
    
    // Iterate over gain limit range
    for (int value1 = -params.gainlimit; value1 <= params.gainlimit; ++value1) {
        double gain = value1 / 10.0;
        
        // Calculate error corrected value
        double ec = alpha_ * (ema_value_ + gain * (price - ec1)) + alpha1_ * ec1;
        
        // Validate the calculated EC value
        if (std::isnan(ec) || !std::isfinite(ec)) {
            continue;  // Skip invalid calculations
        }
        
        // Calculate error
        double error = std::abs(price - ec);
        
        // Track best error correction
        if (error < least_error) {
            least_error = error;
            best_ec = ec;
        }
    }
    
    // Ensure we have a valid result before setting
    if (std::isnan(best_ec) || !std::isfinite(best_ec)) {
        best_ec = ema_value_;  // Fallback to EMA value
    }
    
    ec_line->set(0, best_ec);
}

void ZeroLagIndicator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Try to get data line - check number of lines first to avoid exception
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() > 4) {
        // DataSeries with multiple lines, use Close (index 4)
        data_line = datas[0]->lines->getline(4);
    } else if (datas[0]->lines->size() > 0) {
        // LineSeries with single line, use index 0
        data_line = datas[0]->lines->getline(0);
    } else {
        return;  // No lines available
    }
    
    auto ec_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ec));
    if (!data_line || !ec_buffer) return;
    
    // Don't reset the buffer if it already has data
    if (ec_buffer->size() == 0) {
        // Only reset if buffer is empty
        ec_buffer->reset();
    }
    
    // Initialize EMA state
    double ema_value = 0.0;
    bool ema_init = false;
    
    // Get LineBuffer to access array directly
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    const auto& data_array = data_buffer->array();
    if (data_array.empty()) return;
    
    
    for (int i = start; i < end && i < static_cast<int>(data_array.size()); ++i) {
        // Return NaN for first params.period-1 values to match Python behavior
        if (i < params.period - 1) {
            ec_buffer->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        double price = data_array[i];
        
        // Check for invalid price data
        if (std::isnan(price) || !std::isfinite(price)) {
            ec_buffer->append(std::numeric_limits<double>::quiet_NaN());
            continue;  // Skip this calculation if price is invalid
        }
        
        // Initialize EMA with SMA seed at period-1 index
        if (!ema_init && i == params.period - 1) {
            // Calculate SMA seed for EMA
            double sum = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum += data_array[j];
            }
            ema_value = sum / params.period;
            ema_init = true;
        } else if (ema_init) {
            // Update EMA: ema = alpha * price + alpha1 * previous_ema
            ema_value = alpha_ * price + alpha1_ * ema_value;
        }
        
        // At first valid index (period-1), ec1 is not available, so EC = EMA
        if (i == params.period - 1) {
            // First EC value equals EMA
            ec_buffer->append(ema_value);
            continue;
        }
        
        // Get previous EC value for error correction
        double ec1 = ec_buffer->array().back();
        if (std::isnan(ec1) || !std::isfinite(ec1)) {
            // If previous value is invalid, use EMA
            ec1 = ema_value;
        }
        
        // Error correction optimization
        double least_error = std::numeric_limits<double>::max();
        double best_ec = ema_value;  // Default to EMA value
        
        // Iterate over gain limit range
        for (int value1 = -params.gainlimit; value1 <= params.gainlimit; ++value1) {
            double gain = value1 / 10.0;
            
            // Calculate error corrected value
            double ec = alpha_ * (ema_value + gain * (price - ec1)) + alpha1_ * ec1;
            
            // Validate the calculated EC value
            if (std::isnan(ec) || !std::isfinite(ec)) {
                continue;  // Skip invalid calculations
            }
            
            // Calculate error
            double error = std::abs(price - ec);
            
            // Track best error correction
            if (error < least_error) {
                least_error = error;
                best_ec = ec;
            }
        }
        
        // Ensure we have a valid result before setting
        if (std::isnan(best_ec) || !std::isfinite(best_ec)) {
            best_ec = ema_value;  // Fallback to EMA value
        }
        
        ec_buffer->append(best_ec);
    }
    
    // Finalize the line buffer
    utils::finalizeLineBuffer(ec_buffer);
    
}

} // namespace indicators
} // namespace backtrader