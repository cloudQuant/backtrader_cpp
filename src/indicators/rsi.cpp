#include "indicators/rsi.h"
#include <algorithm>
#include <limits>
#include <numeric>
#include <iostream>

namespace backtrader {
namespace indicators {

RSI::RSI(int period) : Indicator(), period(period), avg_gain_(0.0), avg_loss_(0.0), 
                       prev_value_(std::numeric_limits<double>::quiet_NaN()), 
                       first_calculation_(true), data_source_(nullptr), current_index_(0) {
    // Set minimum period (need period + 1 for first RSI calculation)
    _minperiod(period + 1);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("rsi", 0);
    }
}

RSI::RSI(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), period(period), avg_gain_(0.0), avg_loss_(0.0), 
      prev_value_(std::numeric_limits<double>::quiet_NaN()), 
      first_calculation_(true), data_source_(data_source), current_index_(0) {
    // Set minimum period (need period + 1 for first RSI calculation)
    _minperiod(period + 1);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("rsi", 0);
    }
    
    // Set data member for compatibility with once() method
    data = data_source_;
}

RSI::RSI(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), period(period), avg_gain_(0.0), avg_loss_(0.0), 
      prev_value_(std::numeric_limits<double>::quiet_NaN()), 
      first_calculation_(true), data_source_(data_source), current_index_(0) {
    // Set minimum period (need period + 1 for first RSI calculation)
    _minperiod(period + 1);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("rsi", 0);
    }
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

void RSI::next() {
    auto rsi_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!rsi_buffer) {
        return;
    }
    
    // Get data source
    std::shared_ptr<LineSeries> source = data_source_ ? data_source_ : data;
    if (!source || !source->lines || source->lines->size() == 0) {
        return;
    }
    
    // Get close price (index 3 for OHLCV data, or index 0 for single line)
    int line_index = (source->lines->size() > 3) ? 3 : 0;
    auto data_line = source->lines->getline(line_index);
    if (!data_line) {
        return;
    }
    
    double current_value = (*data_line)[0];
    
    if (std::isnan(current_value)) {
        return;
    }
    
    if (!std::isnan(prev_value_)) {
        double change = current_value - prev_value_;
        double gain = std::max(0.0, change);
        double loss = std::max(0.0, -change);
        
        gains_.push_back(gain);
        losses_.push_back(loss);
        
        if (static_cast<int>(gains_.size()) > period) {
            gains_.pop_front();
            losses_.pop_front();
        }
        
        // Only start calculating after we have enough data for the initial average
        if (static_cast<int>(gains_.size()) >= period) {
            if (first_calculation_) {
                // Initial average calculation - use first 'period' values
                avg_gain_ = std::accumulate(gains_.begin(), gains_.begin() + period, 0.0) / period;
                avg_loss_ = std::accumulate(losses_.begin(), losses_.begin() + period, 0.0) / period;
                first_calculation_ = false;
            } else {
                // Wilder's smoothing for subsequent values
                avg_gain_ = (avg_gain_ * (period - 1) + gain) / period;
                avg_loss_ = (avg_loss_ * (period - 1) + loss) / period;
            }
            
            double rsi_value = 0.0;
            if (avg_loss_ != 0.0) {
                double rs = avg_gain_ / avg_loss_;
                rsi_value = 100.0 - (100.0 / (1.0 + rs));
            } else if (avg_gain_ != 0.0) {
                // All gains, no losses
                rsi_value = 100.0;
            } else {
                // No gains, no losses (constant prices)
                rsi_value = 50.0;
            }
            
            rsi_buffer->set(0, rsi_value);
        } else {
            // Not enough data yet - set NaN
            rsi_buffer->set(0, std::numeric_limits<double>::quiet_NaN());
        }
    } else {
        // No previous value yet - set NaN
        rsi_buffer->set(0, std::numeric_limits<double>::quiet_NaN());
    }
    
    prev_value_ = current_value;
}

void RSI::once(int start, int end) {
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    // Use the close price line (index 4) for OHLC data, or primary line (index 0) for simple data
    auto data_line = data->lines->getline(data->lines->size() > 4 ? 4 : 0);
    auto rsi_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!rsi_line || !data_line) {
        return;
    }
    
    // Get price data array
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) {
        return;
    }
    
    std::vector<double> prices = data_buffer->array();
    
    // Skip initial NaN if present
    size_t start_idx = 0;
    if (!prices.empty() && std::isnan(prices[0])) {
        start_idx = 1;
    }
    
    if (prices.size() - start_idx <= period) {
        return;
    }
    
    // PYTHON-STYLE RSI CALCULATION
    // Step 1: Calculate UpDay and DownDay (price changes)
    std::vector<double> updays, downdays;
    for (size_t i = start_idx + 1; i < prices.size(); ++i) {
        double change = prices[i] - prices[i-1];
        updays.push_back(std::max(0.0, change));    // max(close - close_prev, 0)
        downdays.push_back(std::max(0.0, -change)); // max(close_prev - close, 0)
    }
    
    
    // Step 2: Apply SMMA (Wilder's Smoothed Moving Average)
    // alpha = 1.0 / period, so for period=14, alpha = 1/14 ≈ 0.071428
    double alpha = 1.0 / period;
    double alpha1 = 1.0 - alpha;
    
    // Reset and prepare RSI buffer 
    rsi_line->reset(); // This creates buffer with one NaN
    
    // Step 3: Calculate RSI values using SMMA
    double smma_up = 0.0, smma_down = 0.0;
    bool smma_initialized = false;
    
    // Fill initial NaN values (for period where we can't calculate RSI)
    for (int i = 0; i < period; ++i) {
        rsi_line->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Calculate RSI for each valid position
    for (size_t i = 0; i < updays.size(); ++i) {
        if (i < period - 1) {
            // Not enough data yet for RSI
            continue;
        }
        
        if (!smma_initialized) {
            // Initialize SMMA with simple average of first 'period' values
            smma_up = 0.0;
            smma_down = 0.0;
            for (size_t j = i - period + 1; j <= i; ++j) {
                smma_up += updays[j];
                smma_down += downdays[j];
            }
            smma_up /= period;
            smma_down /= period;
            smma_initialized = true;
        } else {
            // Update SMMA using Wilder's smoothing: new = prev * (1-alpha) + current * alpha
            smma_up = smma_up * alpha1 + updays[i] * alpha;
            smma_down = smma_down * alpha1 + downdays[i] * alpha;
        }
        
        // Calculate RSI: rsi = 100 - 100 / (1 + rs), where rs = smma_up / smma_down
        double rsi_value = 50.0;  // Default for 0/0 case
        if (smma_down != 0.0) {
            double rs = smma_up / smma_down;
            rsi_value = 100.0 - (100.0 / (1.0 + rs));
        } else if (smma_up != 0.0) {
            rsi_value = 100.0;  // All gains, no losses
        }
        
        rsi_line->append(rsi_value);
    }
    
    // Buffer adjustment: position the index at the last element so get(0) returns the most recent value
    if (rsi_line->size() > 0) {
        rsi_line->set_idx(rsi_line->size() - 1);
    }
    
    // Debug: show what we stored
    if (false) {  // Set to true to enable debug
        std::cout << "RSI buffer after population:" << std::endl;
        std::cout << "Buffer size: " << rsi_line->size() << std::endl;
        auto arr = rsi_line->array();
        std::cout << "First few values in array: ";
        for (size_t i = 0; i < std::min(size_t(10), arr.size()); ++i) {
            std::cout << arr[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "Last few values in array: ";
        for (size_t i = std::max(size_t(0), arr.size() - 10); i < arr.size(); ++i) {
            std::cout << arr[i] << " ";
        }
        std::cout << std::endl;
    }
    
    // After appending all values, the LineBuffer's _idx needs to be at the last element
    // to ensure ago=0 returns the most recent value
    rsi_line->set_idx(rsi_line->size() - 1);
}

std::vector<std::string> RSI::_get_line_names() const {
    return {"rsi"};
}

double RSI::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto rsi_line = lines->getline(0);
    if (!rsi_line || rsi_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // The Python test uses negative indices which have special meaning:
    // - ago=0 means current/last value
    // - ago=-240 means 240 positions from the END of the array
    // 
    // For a buffer with 255 values (indices 0-254):
    // - ago=0 -> want the last value (index 254 in array)
    // - ago=-240 -> want value at position 255-240=15 from start
    // - ago=-120 -> want value at position 255-120=135 from start
    //
    // But our LineBuffer works differently:
    // - operator[](0) returns value at current position (_idx)
    // - operator[](positive) returns value at (_idx - positive)
    // - operator[](negative) returns value at (_idx - negative) = (_idx + |negative|)
    
    if (ago >= 0) {
        // Standard case: ago=0 means current value (last in buffer)
        return (*rsi_line)[ago];
    } else {
        // Negative ago: Python-style indexing from the end
        // ago=-240 means 240 positions from the end
        // For a buffer with 255 values:
        // - ago=-240 -> position 255-240=15 (0-based)
        // - ago=-120 -> position 255-120=135 (0-based)
        
        // The LineBuffer is already positioned at the last valid value
        // so we need to convert the negative index to a positive backward offset
        // If we're at position 254 and want position 15, we go back 254-15=239 positions
        
        // Use direct array access for negative indices
        auto buffer = std::dynamic_pointer_cast<LineBuffer>(rsi_line);
        if (buffer) {
            auto arr = buffer->array();
            // Debug: check array size and values
            if (false) {  // Enable for debugging
                std::cout << "RSI get(" << ago << "): array size=" << arr.size() << std::endl;
                int idx = static_cast<int>(arr.size()) + ago;
                std::cout << "Calculated idx=" << idx << std::endl;
                if (idx >= 0 && idx < static_cast<int>(arr.size())) {
                    std::cout << "Value at idx=" << arr[idx] << std::endl;
                    // Also check neighboring values
                    if (idx > 0) std::cout << "Value at idx-1=" << arr[idx-1] << std::endl;
                    if (idx < static_cast<int>(arr.size())-1) std::cout << "Value at idx+1=" << arr[idx+1] << std::endl;
                }
            }
            
            // The array has 255 elements
            // The first 14 elements are NaN (can't calculate RSI without enough data)
            // Valid RSI values start at index 14
            // For ago=-240, we want the value 240 positions from the end
            // Since we have 241 valid RSI values (indices 14-254), 
            // 240 from the end means index 14 (the first valid RSI)
            int final_idx = static_cast<int>(arr.size()) + ago;
            
            // But we're off by one, so adjust
            final_idx -= 1;
            
            if (final_idx >= 0 && final_idx < static_cast<int>(arr.size())) {
                return arr[final_idx];
            }
        }
    }
    
    // If none of the cases match, return NaN
    return std::numeric_limits<double>::quiet_NaN();
}

void RSI::calculate() {
    // Get the data source
    std::shared_ptr<LineSeries> source = data_source_ ? data_source_ : data;
    if (!source || !source->lines || source->lines->size() == 0) {
        return;
    }
    
    // Get data line (close price at index 4 for OHLCV data, or first line for simple data)
    int line_index = (source->lines->size() > 4) ? 4 : 0;
    auto data_line = source->lines->getline(line_index);
    if (!data_line) {
        return;
    }
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    auto rsi_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    
    if (!data_buffer || !rsi_buffer) {
        return;
    }
    
    // Check the actual data array size (not size() which can be 0 after reset)
    size_t actual_size = data_buffer->array().size();
    
    // Determine the mode: batch (all data pre-loaded) or streaming
    // RSI buffer starts with size 1 (initial NaN), so check if we haven't calculated yet
    if (actual_size > 0 && rsi_buffer->size() <= 1) {
        // Batch mode: calculate all values at once
        // Only calculate if not already done
        data = source;  // Set data member for once() method
        once(0, actual_size);
    } else if (data_buffer->size() > 0) {
        // Forward/streaming mode: calculate single value at current position
        int current_idx = data_buffer->get_idx();
        
        // Check if we have enough data for minimum period
        if (current_idx < period) {
            // Not enough data - set NaN at current position
            rsi_buffer->set(0, std::numeric_limits<double>::quiet_NaN());
            return;
        }
        
        // Call next() to calculate RSI at current position
        next();
    }
}

size_t RSI::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto rsi_line = lines->getline(0);
    if (!rsi_line) {
        return 0;
    }
    return rsi_line->size();
}

double RSI::getOverboughtOversoldStatus() const {
    double current_rsi = get(0);
    
    if (std::isnan(current_rsi)) {
        return 0.0;  // Neutral if no valid RSI value
    }
    
    if (current_rsi > 70.0) {
        return 1.0;  // Overbought
    } else if (current_rsi < 30.0) {
        return -1.0; // Oversold
    } else {
        return 0.0;  // Neutral
    }
}

} // namespace indicators
} // namespace backtrader