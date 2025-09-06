#include "indicators/sma.h"
#include <numeric>
#include <iostream>

namespace backtrader {
namespace indicators {

SMA::SMA(int period) : Indicator(), period(period), sum_(0.0), current_index_(0) {
    // Ensure period is at least 1
    if (period < 1) {
        this->period = 1;
    }
    
    // Set minimum period
    _minperiod(this->period);
    
    // Initialize lines (LineSeries system)
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("sma", 0);
    }
    
    // Connect LineSeries lines to IndicatorBase lines_ system
    // IndicatorBase::size() uses lines_[0]->size()
}

SMA::SMA(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), period(period), sum_(0.0), data_source_(data_source), current_index_(0) {
    // Ensure period is at least 1
    if (period < 1) {
        this->period = 1;
        _minperiod(1);  // Explicitly set minperiod to 1
    } else {
        // Set minimum period
        _minperiod(this->period);
    }
    // std::cerr << "SMA constructor: period=" << this->period << ", minperiod_=" << minperiod_ << std::endl;
    
    // Initialize lines (LineSeries system)
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("sma", 0);
    }
    
    // std::cerr << "SMA constructor: data_source lines=" << (data_source ? data_source->lines->size() : 0) << std::endl;
    if (data_source && data_source->lines->size() > 0) {
        auto line = data_source->lines->getline(0);
        if (line) {
            // std::cerr << "SMA constructor: data_source line 0 size=" << line->size() << std::endl;
        }
    }
    
    // Connect LineSeries lines to IndicatorBase lines_ system
    // IndicatorBase::size() uses lines_[0]->size()
    
    // Set data member for compatibility with once() method
    data = data_source_;
    datas.push_back(data_source_);
    _clock = data_source_;  // Set clock to data source
}

SMA::SMA(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), period(30), sum_(0.0), current_index_(0) {
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines (LineSeries system)
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("sma", 0);
    }
    
    // DataSeries is a LineSeries
    data_source_ = std::static_pointer_cast<LineSeries>(data_source);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    this->_clock = data_source;  // Set clock to data source
    
    // Connect LineSeries lines to IndicatorBase lines_ system
    // IndicatorBase::size() uses lines_[0]->size()
}

SMA::SMA(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), period(period), sum_(0.0), current_index_(0) {
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines (LineSeries system)
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("sma", 0);
    }
    
    // DataSeries is a LineSeries
    data_source_ = std::static_pointer_cast<LineSeries>(data_source);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    this->_clock = data_source;  // Set clock to data source
    
    // Connect LineSeries lines to IndicatorBase lines_ system
    // IndicatorBase::size() uses lines_[0]->size()
}

SMA::SMA(IndicatorSourceTag, std::shared_ptr<IndicatorBase> indicator_source, int period) 
    : Indicator(), period(period), sum_(0.0), indicator_source_(indicator_source), current_index_(0) {
    // Set minimum period (base period + source indicator's minimum period - 1)
    // Note: Add 1 to account for the initial NaN in the array
    int source_minperiod = indicator_source ? indicator_source->getMinPeriod() : 1;
    _minperiod(source_minperiod + period);
    
    // Initialize lines (LineSeries system)
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("sma", 0);
    }
    
    // Connect LineSeries lines to IndicatorBase lines_ system
    // IndicatorBase::size() uses lines_[0]->size()
}

void SMA::prenext() {
    static int prenext_count = 0;
    prenext_count++;
    
    // During warm-up period, we still need to collect data but output NaN
    double current_value = std::numeric_limits<double>::quiet_NaN();
    
    if (indicator_source_) {
        // Get value from indicator source
        auto source_line = indicator_source_->getLine(0);
        if (source_line) {
            current_value = source_line->get(0);
        }
    } else {
        // Get value from data source
        std::shared_ptr<LineSeries> source = data_source_ ? data_source_ : data;
        if (!source) {
            if (prenext_count <= 3) {
                // std::cerr << "SMA::prenext() #" << prenext_count << " - no source!" << std::endl;
            }
            return;
        }
        
        // For LineSeries, get the close price (index 4 for OHLCV data, or 0 for simple data)
        int line_index = (source->lines->size() > 4) ? 4 : 0;  // Use close price for OHLCV data
        auto close_line = source->lines->getline(line_index);
        if (close_line && close_line->size() > 0) {
            current_value = (*close_line)[0];
            if (prenext_count <= 3) {
                // std::cerr << "SMA::prenext() #" << prenext_count << " - got value " << current_value << std::endl;
            }
        } else {
            if (prenext_count <= 3) {
                // std::cerr << "SMA::prenext() #" << prenext_count << " - no close_line or size=0" << std::endl;
            }
        }
    }
    
    // Collect valid values for calculation
    if (!std::isnan(current_value)) {
        values_.push_back(current_value);
        sum_ += current_value;
        if (prenext_count <= 3) {
            // std::cerr << "SMA::prenext() #" << prenext_count << " - collected, values_.size()=" << values_.size() << std::endl;
        }
    }
    
    // Always output NaN during warm-up period
    auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (sma_line) {
        sma_line->append(std::numeric_limits<double>::quiet_NaN());
    }
}

void SMA::nextstart() {
    // std::cerr << "SMA::nextstart() called - values_.size()=" << values_.size() << ", period=" << period << std::endl;
    
    // Called once when minimum period is reached
    std::shared_ptr<LineSeries> source = data_source_ ? data_source_ : data;
    if (!source) {
        // std::cerr << "SMA::nextstart() - no source!" << std::endl;
        return;
    }
    
    // In streaming mode, we need to collect all historical values up to the current point
    // because prenext may not have been called enough times
    int line_index = (source->lines->size() > 4) ? 4 : 0;  // Use close price for OHLCV data
    auto close_line = source->lines->getline(line_index);
    if (close_line && close_line->size() > 0) {
        // If we don't have enough values yet, collect them from the buffer
        if (static_cast<int>(values_.size()) < period) {
            auto linebuf = std::dynamic_pointer_cast<LineBuffer>(close_line);
            if (linebuf) {
                const auto& data_array = linebuf->array();
                int current_idx = linebuf->get_idx();
                
                // Collect values from the beginning up to current position
                values_.clear();
                sum_ = 0.0;
                
                // We need 'period' values ending at current_idx
                int start_idx = std::max(0, current_idx - period + 1);
                for (int i = start_idx; i <= current_idx; ++i) {
                    if (i >= 0 && i < static_cast<int>(data_array.size())) {
                        double value = data_array[i];
                        if (!std::isnan(value)) {
                            values_.push_back(value);
                            sum_ += value;
                        }
                    }
                }
                // std::cerr << "SMA::nextstart() - collected " << values_.size() << " values from buffer" << std::endl;
            }
        }
    }
    
    // Calculate and output the first SMA value
    double sma_value = std::numeric_limits<double>::quiet_NaN();
    if (static_cast<int>(values_.size()) >= period) {
        sma_value = sum_ / period;
        // std::cerr << "SMA::nextstart() - calculated sma_value=" << sma_value << std::endl;
    } else {
        // std::cerr << "SMA::nextstart() - not enough values: " << values_.size() << " < " << period << std::endl;
    }
    
    auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (sma_line) {
        sma_line->append(sma_value);
    }
}

void SMA::next() {
    // Called for subsequent calculations after nextstart
    double current_value = std::numeric_limits<double>::quiet_NaN();
    
    if (indicator_source_) {
        // Get value from indicator source
        auto source_line = indicator_source_->getLine(0);
        if (source_line) {
            current_value = source_line->get(0);
        }
    } else {
        // Get value from data source
        std::shared_ptr<LineSeries> source = data_source_ ? data_source_ : data;
        if (!source) {
            return;
        }
        
        // Get current value from close price (index 4 for OHLCV data, or 0 for simple data)
        int line_index = (source->lines->size() > 4) ? 4 : 0;  // Use close price for OHLCV data
        auto close_line = source->lines->getline(line_index);
        if (!close_line || close_line->size() == 0) {
            return;
        }
        
        current_value = (*close_line)[0];
    }
    
    if (std::isnan(current_value)) {
        auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
        if (sma_line) {
            sma_line->append(std::numeric_limits<double>::quiet_NaN());
        }
        return;
    }
    
    // Update sliding window
    values_.push_back(current_value);
    sum_ += current_value;
    
    if (static_cast<int>(values_.size()) > period) {
        sum_ -= values_.front();
        values_.pop_front();
    }
    
    // Calculate SMA
    double sma_value = std::numeric_limits<double>::quiet_NaN();
    if (static_cast<int>(values_.size()) >= period) {
        sma_value = sum_ / period;
    }
    
    // Output the value
    auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (sma_line) {
        sma_line->append(sma_value);
    }
}

void SMA::once(int start, int end) {
    // Determine the correct data line based on the data source type
    std::shared_ptr<LineSingle> data_line;
    
    if (indicator_source_) {
        // When using an indicator as source, get its output line
        data_line = indicator_source_->getLine(0);
        if (!data_line) {
            return;
        }
    } else {
        // Support both data_source_ (LineSeries) and data (DataSeries) members
        std::shared_ptr<LineSeries> source = data_source_ ? data_source_ : data;
        if (!source || source->lines->size() == 0) {
            // std::cerr << "SMA::once() - no data!" << std::endl;
            return;
        }
        
        // Check if this is a DataSeries (has multiple lines including OHLCV)
        if (source->lines->size() > 4) {
            // For DataSeries: 0=DateTime, 1=Open, 2=High, 3=Low, 4=Close, 5=Volume, 6=OpenInterest
            data_line = source->lines->getline(4);  // Close line
        } else {
            // For LineSeries or single line data: use line 0
            data_line = source->lines->getline(0);
        }
    }
    
    // Debug: Check what we're processing
    static int once_debug = 0;
    if (once_debug++ == 0) {
        auto linebuf = std::dynamic_pointer_cast<LineBuffer>(data_line);
        if (linebuf) {
            std::cout << "SMA::once() debug:\n";
            std::cout << "  start=" << start << ", end=" << end << "\n";
            std::cout << "  Data array size: " << linebuf->array().size() << "\n";
            std::cout << "  Processing " << (end - start) << " values\n";
        }
    }
    
    auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!sma_line || !data_line) {
        // std::cerr << "SMA::once() - no lines! sma_line=" << sma_line.get() 
        //           << ", data_line=" << data_line.get() << std::endl;
        return;
    }
    
    
    // In batch mode, we build the output array from scratch
    // Don't use reset() as it adds an initial NaN that causes off-by-1 errors
    
    // Get actual data size to avoid out-of-bounds access
    auto linebuf = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!linebuf) {
        return;
    }
    const auto& data_array = linebuf->array();
    int data_size = static_cast<int>(data_array.size());
    
    // Adjust end to not exceed data size
    end = std::min(end, data_size);
    
    // Build a temporary array with the SMA values
    std::vector<double> sma_values;
    if (end > start) {
        sma_values.reserve(end - start);
    }
    
    // Process data in forward order (matching Python's behavior)
    for (int i = start; i < end; ++i) {
        // Calculate SMA for position i using the window [i-period+1, i]
        double window_sum = 0.0;
        int valid_count = 0;
        
        
        // Calculate the range for this position
        // When processing from start > 0 (nested indicators), adjust window accordingly
        int window_start = std::max(0, i - period + 1);
        int window_end = i + 1;
        
        // Debug output for first few calculations
        if (i < start + 3) {
            std::cerr << "SMA::once() - i=" << i << ", window_start=" << window_start 
                      << ", window_end=" << window_end << ", period=" << period << std::endl;
        }
        
        
        // Sum up the window
        // In runonce mode, data is pre-loaded and we access it directly by index
        for (int j = window_start; j < window_end; ++j) {
            // Access the value at absolute position j using array()
            // data_array was already obtained above
            if (j >= 0 && j < data_size) {
                double value = data_array[j];
                
                if (!std::isnan(value)) {
                    window_sum += value;
                    valid_count++;
                    
                }
                
            }
        }
        
        // Calculate SMA value
        double result_value;
        // Check if we have enough data points for SMA calculation
        // For SMA with period=30, we need at least 30 values
        // The first valid SMA value is at index period-1 (29 for period=30)
        if (valid_count == period && i >= period - 1) {
            result_value = window_sum / period;
        } else {
            result_value = std::numeric_limits<double>::quiet_NaN();
        }
        
        
        // Add to temporary array
        sma_values.push_back(result_value);
    }
    
    // Now replace the SMA line buffer with our calculated values
    // Use reset() instead of clear() to maintain proper initial state
    sma_line->reset();
    
    // Debug: Check buffer state after reset
    static int reset_debug = 0;
    if (reset_debug++ == 0) {
        std::cout << "SMA::once() after reset(): buffer size = " << sma_line->array().size() << "\n";
    }
    
    // Debug: Check how many values we're appending
    static int debug_once = 0;
    if (debug_once++ < 3) {
        std::cout << "SMA::once() period=" << period << ", appending " << sma_values.size() << " values to buffer\n";
        // Also check some values at different positions
        std::cout << "  Values at key positions:\n";
        for (size_t j = 0; j < sma_values.size(); j += 10) {
            std::cout << "    [" << j << "]=" << sma_values[j] << "\n";
        }
    }
    
    // Append all our calculated values (first one will overwrite the initial NaN from reset)
    static int loop_debug = 0;
    int append_count = 0;
    for (size_t i = 0; i < sma_values.size(); ++i) {
        if (i == 0) {
            // First value: set directly at index 0 to overwrite the initial NaN
            sma_line->set(0, sma_values[i]);
        } else {
            // Subsequent values: append normally
            sma_line->append(sma_values[i]);
        }
        append_count++;
    }
    if (loop_debug++ == 0) {
        std::cout << "SMA::once() set/appended " << append_count << " values in loop\n";
    }
    
    // Set LineBuffer index to last valid position for proper ago indexing
    if (end > start && sma_line->array().size() > 0) {
        static int setidx_debug = 0;
        if (setidx_debug++ == 0) {
            std::cout << "SMA::once() before set_idx: buffer size = " << sma_line->array().size() << "\n";
        }
        sma_line->set_idx(sma_line->array().size() - 1);
        if (setidx_debug == 1) {
            std::cout << "SMA::once() after set_idx: buffer size = " << sma_line->array().size() << "\n";
            // Check what's at the current position
            std::cout << "  Value at current idx: " << sma_line->get(0) << "\n";
            std::cout << "  Value at idx -1: " << sma_line->get(-1) << "\n";
            std::cout << "  Value at idx -2: " << sma_line->get(-2) << "\n";
        }
    }
    
    // Debug: Check final state
    static int final_debug = 0;
    if (final_debug++ == 0) {
        std::cout << "SMA::once() final state:\n";
        std::cout << "  SMA buffer size after once(): " << sma_line->array().size() << "\n";
    }
    
    // CRITICAL: After calculating all values in runonce mode,
    // leave idx at the end position so future calls can navigate properly
    // Do NOT set idx here - let the strategy manage positioning
}

void SMA::_once() {
    std::cerr << "SMA::_once() called! period=" << period << std::endl;
    
    // Get data's buflen since indicators use data's buflen in runonce mode
    size_t data_buflen = 0;
    if (data) {
        data_buflen = data->buflen();
    }
    
    std::cerr << "SMA::_once() - data_buflen=" << data_buflen 
              << ", minperiod_=" << minperiod_ 
              << ", period=" << period << std::endl;
    
    // DON'T forward - this moves the index past calculated values
    // Just calculate all values at the current position
    
    // No child indicators to process for SMA
    
    // Execute the three phases of once processing
    // Handle case where data_buflen < minperiod_
    if (data_buflen < minperiod_) {
        // Not enough data for SMA calculation
        std::cerr << "SMA::_once() - Not enough data, calling preonce(0, " << data_buflen << ")" << std::endl;
        preonce(0, data_buflen);
    } else {
        // For SMA, we need to calculate all values from the beginning
        // not just from minperiod onwards
        std::cerr << "SMA::_once() - Calculating all values, calling once(0, " << data_buflen << ")" << std::endl;
        once(0, data_buflen);
    }
    
    // After calculating, position at the last valid index
    auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (sma_line && sma_line->array().size() > 0) {
        sma_line->set_idx(sma_line->array().size() - 1);
        std::cerr << "SMA::_once() - Positioned at index " << sma_line->get_idx() << std::endl;
    }
    
    // Execute binding synchronization if any
    for (auto& line : lines_) {
        line->oncebinding();
    }
}

std::vector<std::string> SMA::_get_line_names() const {
    return {"sma"};
}

size_t SMA::size() const {
    static int debug_count = 0;
    debug_count++;
    
    if (!lines || lines->size() == 0) {
        if (debug_count <= 10) {
            // std::cerr << "SMA::size() #" << debug_count << " - no lines, returning 0" << std::endl;
        }
        return 0;
    }
    
    auto sma_line = lines->getline(0);
    if (!sma_line) {
        if (debug_count <= 10) {
            // std::cerr << "SMA::size() #" << debug_count << " - no sma_line, returning 0" << std::endl;
        }
        return 0;
    }
    
    size_t sz = sma_line->size();
    if (debug_count <= 20) {
        // std::cerr << "SMA::size() #" << debug_count << " - returning " << sz << ", this=" << this << std::endl;
    }
    return sz;
}

double SMA::get(int ago) const {
    static int get_count = 0;
    get_count++;
    
    if (!lines || lines->size() == 0) {
        if (get_count <= 3) {
            // std::cerr << "SMA::get() #" << get_count << " - no lines, returning NaN" << std::endl;
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto sma_line = lines->getline(0);
    if (!sma_line || sma_line->size() == 0) {
        if (get_count <= 3) {
            // std::cerr << "SMA::get() #" << get_count << " - no sma_line or size=0, returning NaN" << std::endl;
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // In runonce mode, we need special handling
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(sma_line);
    if (buffer) {
        // Check if we're in runonce mode (buffer has all values pre-calculated)
        // In runonce mode, buflen() will be much larger than size()
        // because once() calculates all values at once
        size_t buflen = buffer->buflen();
        size_t cur_size = buffer->size();
        int idx = buffer->get_idx();
        
        if (get_count <= 30) {
            std::cerr << "SMA::get(" << ago << ") #" << get_count 
                      << " - idx=" << idx
                      << ", size=" << cur_size 
                      << ", buflen=" << buflen << std::endl;
        }
        
        // In runonce mode with positioned buffer (used during Strategy::once())
        // The buffer's idx tells us which bar we're currently at
        // For lookback (ago > 0), we need to access idx - ago
        // Check if we're in runonce mode with a valid positioned index
        bool is_runonce_positioned = (buflen > 1 && idx >= 0 && idx < static_cast<int>(buflen) && ago >= 0);
        
        if (is_runonce_positioned) {
            // We're in runonce mode with a positioned buffer
            // Access the value at the correct position
            const auto& array = buffer->array();
            
            // Calculate the actual position to access
            int target_idx = idx - ago;
            
            // Check bounds
            if (target_idx >= 0 && target_idx < static_cast<int>(array.size())) {
                double result = array[target_idx];
                if (get_count <= 30) {
                    std::cerr << "SMA::get(" << ago << ") #" << get_count 
                              << " - runonce mode, target_idx=" << target_idx 
                              << ", result=" << result << std::endl;
                }
                return result;
            } else {
                // Out of bounds - return NaN
                if (get_count <= 30) {
                    std::cerr << "SMA::get(" << ago << ") #" << get_count 
                              << " - runonce mode, target_idx=" << target_idx 
                              << " out of bounds, returning NaN" << std::endl;
                }
                return std::numeric_limits<double>::quiet_NaN();
            }
        }
    }
    
    // Fall back to standard LineBuffer indexing for non-runonce mode or negative indices
    // LineBuffer already implements Python indexing convention correctly
    // Just pass the ago value directly
    double result = (*sma_line)[ago];
    
    if (get_count <= 30) {
        std::cerr << "SMA::get(" << ago << ") #" << get_count << " - standard mode, result=" << result << std::endl;
    }
    
    return result;
}

void SMA::calculate() {
    static int calc_count = 0;
    calc_count++;
    
    // Debug: Track calls
    if (calc_count <= 5) {
        std::cout << "SMA::calculate() call #" << calc_count << "\n";
    }
    
    // Handle different data sources
    std::shared_ptr<LineSingle> data_line;
    
    if (indicator_source_) {
        // When using an indicator as source, get its output line
        // std::cerr << "SMA::calculate() NEW #" << calc_count << " - using indicator_source_" << std::endl;
        
        try {
            // Safely get the line from the indicator source
            if (!indicator_source_) {
                // std::cerr << "SMA::calculate() NEW #" << calc_count << " - indicator_source_ is null!" << std::endl;
                auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
                if (sma_line) {
                    sma_line->append(std::numeric_limits<double>::quiet_NaN());
                }
                return;
            }
            
            // Debug: Check indicator source state
            // std::cerr << "SMA::calculate() NEW #" << calc_count << " - indicator_source_ has lines: " 
            //           << (indicator_source_->lines ? "yes" : "no") << std::endl;
            if (indicator_source_->lines) {
                // std::cerr << "SMA::calculate() NEW #" << calc_count << " - indicator_source_ lines size: " 
                //           << indicator_source_->lines->size() << std::endl;
            }
            
            data_line = indicator_source_->getLine(0);  // Get the first output line of the indicator
            if (!data_line) {
                // std::cerr << "SMA::calculate() NEW #" << calc_count << " - no data_line from indicator!" << std::endl;
                // Append NaN when source has no data yet
                auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
                if (sma_line) {
                    sma_line->append(std::numeric_limits<double>::quiet_NaN());
                }
                return;
            }
        } catch (const std::exception& e) {
            // std::cerr << "SMA::calculate() NEW #" << calc_count << " - exception getting line from indicator: " << e.what() << std::endl;
            
            // Append NaN when there's an error
            auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
            if (sma_line) {
                sma_line->append(std::numeric_limits<double>::quiet_NaN());
            }
            return;
        }
    } else {
        // Support both data_source_ (LineSeries) and data (DataSeries) members
        std::shared_ptr<LineSeries> source = data_source_ ? data_source_ : data;
        // std::cerr << "SMA::calculate() NEW #" << calc_count << " - data_source_=" << (data_source_ ? "set" : "null") 
        //           << ", data=" << (data ? "set" : "null") << std::endl;
        
        if (!source) {
            // std::cerr << "SMA::calculate() NEW #" << calc_count << " - no source data!" << std::endl;
            return;
        }
        
        // Get the data line
        // Check if this is a DataSeries (has multiple lines including OHLCV)
        if (source->lines->size() > 4) {
            // For DataSeries: 0=DateTime, 1=Open, 2=High, 3=Low, 4=Close, 5=Volume, 6=OpenInterest
            data_line = source->lines->getline(4);  // Close line
        } else {
            // For LineSeries or single line data: use line 0
            data_line = source->lines->getline(0);
        }
    }
    
    if (!data_line) {
        // std::cerr << "SMA::calculate() NEW #" << calc_count << " - no data_line!" << std::endl;
        return;
    }
    
    // Check if we have a LineBuffer to determine if data is pre-loaded
    auto linebuf = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!linebuf) {
        // std::cerr << "SMA::calculate() NEW #" << calc_count << " - no LineBuffer!" << std::endl;
        return;
    }
    
    // When using indicator source, check if it has any data yet
    if (indicator_source_ && linebuf->size() == 0) {
        // std::cerr << "SMA::calculate() NEW #" << calc_count << " - indicator source has no data yet, appending NaN" << std::endl;
        auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
        if (sma_line) {
            sma_line->append(std::numeric_limits<double>::quiet_NaN());
        }
        return;
    }
    
    // Get the actual data size (not the current position)
    size_t data_buflen = linebuf->buflen();
    // std::cerr << "SMA::calculate() NEW #" << calc_count << " - data_buflen=" << data_buflen 
    //           << ", minperiod_=" << minperiod_ << std::endl;
    
    // Also check the array size directly
    const auto& data_array = linebuf->array();
    size_t array_size = data_array.size();
    // std::cerr << "SMA::calculate() NEW #" << calc_count << " - array_size=" << array_size << std::endl;
    
    // For test framework compatibility: check data_size() which gives total data loaded
    size_t total_data_size = linebuf->data_size();
    // std::cerr << "SMA::calculate() NEW #" << calc_count << " - data_size()=" << total_data_size << std::endl;
    
    // Special handling for nested indicators: when using indicator source,
    // we should process all available data from the source indicator
    if (indicator_source_) {
        // For nested indicators, use batch mode if source has complete data
        // Process nested indicator data
        
        // Debug output
        static int nested_debug = 0;
        if (nested_debug++ == 0) {
            std::cout << "SMA::calculate() nested indicator mode:\n";
            std::cout << "  array_size: " << array_size << "\n";
            std::cout << "  minperiod_: " << minperiod_ << "\n";
        }
        
        // Check if the first element is the initial NaN
        bool has_initial_nan = false;
        if (array_size > 0 && std::isnan(data_array[0])) {
            has_initial_nan = true;
        }
        
        if (array_size > 1) {  // More than just initial NaN
            // Skip the initial NaN if present
            int start_idx = has_initial_nan ? 1 : 0;
            int end_idx = static_cast<int>(array_size);
            
            if (nested_debug == 1) {
                std::cout << "  Calling once(" << start_idx << ", " << end_idx << ")\n";
            }
            
            // Use batch processing for the complete series
            SMA::once(start_idx, end_idx);
        } else {
            // Source indicator doesn't have data yet, output NaN
            auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
            if (sma_line) {
                sma_line->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    } else {
        // If we have pre-loaded data (batch mode), use once() for efficient calculation
        // Check if we have more data than just the initial NaN
        if (total_data_size > static_cast<size_t>(period)) {
            // Debug: Check what we're calculating
            static int calc_debug = 0;
            if (calc_debug++ == 0) {
                std::cout << "SMA::calculate() batch mode:\n";
                std::cout << "  total_data_size: " << total_data_size << "\n";
                std::cout << "  Calling once(0, " << total_data_size << ")\n";
            }
            // Use data_size() for the actual data count
            once(0, static_cast<int>(total_data_size));
            
            // Debug: Check buffer size after once()
            static int after_once_debug = 0;
            if (after_once_debug++ == 0) {
                auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
                if (sma_line) {
                    std::cout << "SMA::calculate() after once(): buffer size = " << sma_line->array().size() << "\n";
                }
            }
        } else {
            // Streaming mode - use the phase-based approach
            size_t current_len = data_line->size();
            // std::cerr << "SMA::calculate() NEW #" << calc_count << " - streaming mode, current_len=" << current_len << std::endl;
            
            if (current_len < minperiod_) {
                // std::cerr << "SMA::calculate() NEW #" << calc_count << " - calling prenext()" << std::endl;
                prenext();
            } else if (current_len == minperiod_) {
                // std::cerr << "SMA::calculate() NEW #" << calc_count << " - calling nextstart()" << std::endl;
                nextstart();
            } else {
                // std::cerr << "SMA::calculate() NEW #" << calc_count << " - calling next()" << std::endl;
                next();
            }
        }
    }
}

} // namespace indicators
} // namespace backtrader