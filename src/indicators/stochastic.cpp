#include "indicators/stochastic.h"
#include "functions.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <typeinfo>

namespace backtrader {

// StochasticBase implementation
StochasticBase::StochasticBase() : Indicator() {
    _minperiod(params.period);
}

void StochasticBase::prenext() {
    // Before minimum period: ensure result lines contain NaN
    static int prenext_count = 0;
    prenext_count++;
    if (lines->size() >= 1) {
        auto k_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percK));
        if (k_line) {
            // In streaming mode, append NaN to the result buffer
            k_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    if (lines->size() >= 2) {
        auto d_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percD));
        if (d_line) {
            // In streaming mode, append NaN to the result buffer
            d_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
}

void StochasticBase::next() {
    // Call the derived class calculate_lines implementation
    static int next_count = 0;
    next_count++;
    calculate_lines();
}

void StochasticBase::calculate() {
    // Debug
    static int base_calc_count = 0;
    base_calc_count++;
    if (base_calc_count >= 15 && base_calc_count <= 18) {
        std::cerr << "StochasticBase::calculate() called, count=" << base_calc_count << std::endl;
    }
    
    // Determine if we're before or after minimum period
    if (datas.empty()) return;
    
    auto first_data = datas[0];
    if (!first_data || !first_data->lines) return;
    
    auto first_line = first_data->lines->getline(0);
    if (!first_line) return;
    
    auto line_buffer = std::dynamic_pointer_cast<LineBuffer>(first_line);
    if (!line_buffer) return;
    
    // Determine current position
    int current_size = 0;
    if (line_buffer->size() == 0) {
        // Streaming mode - use array size
        current_size = static_cast<int>(line_buffer->array().size());
        // Adjust for initial NaN
        if (current_size > 0 && std::isnan(line_buffer->array()[0])) {
            current_size--;
        }
    } else {
        // Forward mode
        current_size = line_buffer->get_idx() + 1;
    }
    
    if (current_size < minperiod_) {
        static int prenext_debug_count = 0;
        prenext_debug_count++;
        if (prenext_debug_count <= 5 && current_size >= 14) {
            std::cerr << "Stochastic calling prenext() - current_size=" << current_size 
                      << ", minperiod=" << minperiod_ << std::endl;
        }
        prenext();
    } else {
        static int next_debug_count = 0;
        next_debug_count++;
        if (next_debug_count <= 5) {
            std::cerr << "Stochastic calling next() - current_size=" << current_size 
                      << ", minperiod=" << minperiod_ << std::endl;
        }
        next();
    }
}

void StochasticBase::once(int start, int end) {
    // Base class does nothing - derived classes should override
}

double StochasticBase::get_highest(int period, int offset) {
    static int debug_call = 0;
    debug_call++;
    
    if (datas.empty() || !datas[0]->lines) return std::numeric_limits<double>::quiet_NaN();
    
    // Check if we have separate line series for each component
    // Constructor takes (high, low, close) and adds in that order
    // So datas[0]=high, datas[1]=low, datas[2]=close
    std::shared_ptr<LineRoot> high_line;
    if (datas.size() >= 3) {
        // If we have multiple data sources, first one is high
        high_line = datas[0]->lines->getline(0);
    } else {
        // If single data source, high is at index 1
        high_line = datas[0]->lines->getline(1);
    }
    
    if (!high_line) return std::numeric_limits<double>::quiet_NaN();
    
    // Check if we need to use array() method for LineBuffer
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    std::vector<double> high_array;
    bool use_array = false;
    
    // In streaming mode, always use array access for correct indexing
    if (high_buffer) {
        high_array = high_buffer->array();
        use_array = true;
    }
    
    double highest = -std::numeric_limits<double>::max();
    if (use_array && !high_array.empty()) {
        int array_size = static_cast<int>(high_array.size());
        int idx = offset;  // offset is already the correct index in the array
        
        
        if (idx < 0 || idx >= array_size) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        // Make sure we have enough data for the period
        int start_idx = std::max(0, idx - period + 1);
        if (start_idx > idx) return std::numeric_limits<double>::quiet_NaN();
        
        
        // Find highest in the period ending at idx
        for (int i = start_idx; i <= idx; ++i) {
            if (i >= 0 && i < array_size) {
                double value = high_array[i];
                if (!std::isnan(value) && value > highest) {
                    highest = value;
                }
            }
        }
    } else if (high_buffer) {
        // Use data_size() if size() > 0
        int data_size = static_cast<int>(high_buffer->data_size());
        if (data_size > 0) {
            highest = (*high_buffer)[offset];
            for (int i = 1; i < period && i + offset < data_size; ++i) {
                double value = (*high_buffer)[i + offset];
                if (!std::isnan(value) && value > highest) {
                    highest = value;
                }
            }
        }
    }
    
    return (highest == -std::numeric_limits<double>::max()) ? std::numeric_limits<double>::quiet_NaN() : highest;
}

double StochasticBase::get_lowest(int period, int offset) {
    if (datas.empty() || !datas[0]->lines) return std::numeric_limits<double>::quiet_NaN();
    
    // Check if we have separate line series for each component
    // Constructor takes (high, low, close) and adds in that order
    // So datas[0]=high, datas[1]=low, datas[2]=close
    std::shared_ptr<LineRoot> low_line;
    if (datas.size() >= 3) {
        // If we have multiple data sources, second one is low
        low_line = datas[1]->lines->getline(0);
    } else {
        // If single data source, low is at index 3 (DataSeries: datetime=0, open=1, high=2, low=3, close=4, volume=5, openinterest=6)
        low_line = datas[0]->lines->getline(3);
    }
    
    if (!low_line) return std::numeric_limits<double>::quiet_NaN();
    
    // Check if we need to use array() method for LineBuffer
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    std::vector<double> low_array;
    bool use_array = false;
    
    // In streaming mode, always use array access for correct indexing
    if (low_buffer) {
        low_array = low_buffer->array();
        use_array = true;
    }
    
    double lowest = std::numeric_limits<double>::max();
    if (use_array && !low_array.empty()) {
        int array_size = static_cast<int>(low_array.size());
        int idx = offset;  // offset is already the correct index in the array
        if (idx < 0 || idx >= array_size) return std::numeric_limits<double>::quiet_NaN();
        
        // Make sure we have enough data for the period
        int start_idx = std::max(0, idx - period + 1);
        if (start_idx > idx) return std::numeric_limits<double>::quiet_NaN();
        
        // Find lowest in the period ending at idx
        for (int i = start_idx; i <= idx; ++i) {
            if (i >= 0 && i < array_size) {
                double value = low_array[i];
                if (!std::isnan(value) && value < lowest) {
                    lowest = value;
                }
            }
        }
    } else if (low_buffer) {
        // Use data_size() if size() > 0
        int data_size = static_cast<int>(low_buffer->data_size());
        if (data_size > 0) {
            lowest = (*low_buffer)[offset];
            for (int i = 1; i < period && i + offset < data_size; ++i) {
                double value = (*low_buffer)[i + offset];
                if (!std::isnan(value) && value < lowest) {
                    lowest = value;
                }
            }
        }
    }
    
    return (lowest == std::numeric_limits<double>::max()) ? std::numeric_limits<double>::quiet_NaN() : lowest;
}

// StochasticFast implementation
StochasticFast::StochasticFast() : StochasticBase() {
    setup_lines();
    
    // Create SMA for %D calculation
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    
    _minperiod(params.period + params.period_dfast - 1);
}

void StochasticFast::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void StochasticFast::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Check if we have separate line series for each component
    // Constructor takes (high, low, close) and adds in that order
    // So datas[0]=high, datas[1]=low, datas[2]=close
    std::shared_ptr<LineRoot> close_line;
    if (datas.size() >= 3) {
        // If we have multiple data sources, third one is close
        close_line = datas[2]->lines->getline(0);
    } else {
        // If single data source, close is at index 3
        close_line = datas[0]->lines->getline(3);
    }
    
    auto k_line = lines->getline(percK);
    auto d_line = lines->getline(percD);
    
    if (!close_line || !k_line || !d_line) return;
    
    // Calculate %K
    double highest_high = get_highest(params.period);
    double lowest_low = get_lowest(params.period);
    
    // Get current close value using same pattern as get_highest/get_lowest
    double current_close;
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    if (close_buffer && close_buffer->size() == 0) {
        auto close_array = close_buffer->array();
        if (!close_array.empty()) {
            current_close = close_array.back();
        } else {
            return;
        }
    } else {
        auto close_buffer_direct = std::dynamic_pointer_cast<LineBuffer>(close_line);
        if (close_buffer_direct && close_buffer_direct->size() > 0) {
            current_close = (*close_buffer_direct)[0];
        } else {
            return;
        }
    }
    
    double k_value = 0.0;
    if (highest_high != lowest_low) {
        k_value = 100.0 * (current_close - lowest_low) / (highest_high - lowest_low);
    } else if (params.safediv) {
        k_value = params.safezero;
    }
    
    k_line->set(0, k_value);
    
    // Store k value for SMA calculation
    k_values_.push_back(k_value);
    if (k_values_.size() > params.period_dfast) {
        k_values_.erase(k_values_.begin());
    }
    
    // Calculate %D as SMA of %K
    if (k_values_.size() >= params.period_dfast) {
        double sum = 0.0;
        for (double val : k_values_) {
            sum += val;
        }
        double d_value = sum / params.period_dfast;
        d_line->set(0, d_value);
    }
}

// Stochastic implementation
Stochastic::Stochastic() : StochasticBase() {
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    // For slow stochastic, we need to consider both %K and %D:
    // %K needs: period + period_dfast - 1
    // %D needs: period + period_dfast + period_dslow - 2
    // Report the maximum since both lines need to be ready
    int min_period = params.period + params.period_dfast + params.period_dslow - 2;
    _minperiod(min_period);
}


Stochastic::Stochastic(std::shared_ptr<LineSeries> data_source, int period) : Stochastic() {
    params.period = period;
    setup_lines();
    // For slow stochastic, report minimum period for %D line
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

Stochastic::Stochastic(std::shared_ptr<DataSeries> data_source) : Stochastic() {
    setup_lines();
    // For slow stochastic, report minimum period for %D line
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

Stochastic::Stochastic(std::shared_ptr<DataSeries> data_source, int period) : Stochastic() {
    params.period = period;
    setup_lines();
    // For slow stochastic, report minimum period for %D line
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}


Stochastic::Stochastic(std::shared_ptr<LineSeries> high_line, 
                       std::shared_ptr<LineSeries> low_line,
                       std::shared_ptr<LineSeries> close_line,
                       int period, int period_dfast, int period_dslow) 
    : StochasticBase() {
    // Set parameters - ensure we're setting the derived class params
    this->params.period = period;
    this->params.period_dfast = period_dfast;
    this->params.period_dslow = period_dslow;
    
    // Also set base class params for compatibility
    StochasticBase::params.period = period;
    StochasticBase::params.period_dfast = period_dfast;
    
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    // For slow stochastic, we need to consider both %K and %D:
    // %K needs: period + period_dfast - 1
    // %D needs: period + period_dfast + period_dslow - 2
    // Use the maximum of the two
    int calculated_minperiod = params.period + params.period_dfast + params.period_dslow - 2;
    _minperiod(calculated_minperiod);
    
    // Add the three data sources in the expected order: high, low, close
    this->datas.push_back(high_line);
    this->datas.push_back(low_line);
    this->datas.push_back(close_line);
}

void Stochastic::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Stochastic::calculate_lines() {
    // Debug output
    static int debug_call_count = 0;
    debug_call_count++;
    
    // Check if this is being called from calculate() in streaming mode
    // If so, we need to handle it differently
    
    // For streaming mode: calculate single value at current position
    if (datas.size() >= 3) {
        auto high_line = datas[0]->lines->getline(0);
        auto low_line = datas[1]->lines->getline(0);
        auto close_line = datas[2]->lines->getline(0);
        
        if (!high_line || !low_line || !close_line) {
            return;
        }
        
        auto k_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percK));
        auto d_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percD));
        
        if (!k_line || !d_line) {
            return;
        }
        
        // Check if we have enough data for minimum period
        auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
        int current_data_size = 0;
        int current_idx = 0;
        bool has_initial_nan = false;
        
        if (high_buffer) {
            const auto& arr = high_buffer->array();
            has_initial_nan = (arr.size() > 0 && std::isnan(arr[0]));
            
            if (high_buffer->size() == 0) {
                // Streaming mode: after reset(), buffers have been cleared
                // The array size tells us how many data points have been appended
                current_data_size = static_cast<int>(arr.size());
                
                // In test's streaming mode, the buffer was reset (no initial NaN)
                // So arr.size() directly represents the number of real data points
                if (has_initial_nan && current_data_size > 0) {
                    // If there's still an initial NaN (which shouldn't happen after reset), adjust
                    current_data_size--;
                }
                
                // Current index is the last valid data index
                current_idx = current_data_size > 0 ? current_data_size - 1 : 0;
            } else {
                // Forward mode: use current position from get_idx()
                current_idx = high_buffer->get_idx();
                current_data_size = current_idx + 1;  // data points from 0 to current_idx
                if (has_initial_nan) {
                    current_data_size--;  // Adjust for initial NaN
                }
            }
            
            // Debug output disabled
        }
        
        // Check minimum period requirement
        // For %K line: period + period_dfast - 1
        // For %D line: period + period_dfast + period_dslow - 2
        int k_min_period = params.period + params.period_dfast - 1;
        int d_min_period = params.period + params.period_dfast + params.period_dslow - 2;
        
        // In streaming mode, calc_count should represent how many real data points we have
        // In the test loop: step i means we have i+1 data points (0-based index to 1-based count)
        // The LineBuffer starts with initial NaN, so after appending n values, size() = n+1
        // current_data_size is already adjusted (size - 1 if has_initial_nan)
        // So current_data_size correctly represents the number of real data points
        int calc_count = current_data_size;
        
        
        // For raw %K calculation, we need at least 'period' data points
        if (current_data_size < params.period) {
            // Not enough data - append NaN values
            k_line->append(std::numeric_limits<double>::quiet_NaN());
            d_line->append(std::numeric_limits<double>::quiet_NaN());
            return;
        }
        
        // Calculate raw %K for current position using direct array access
        auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
        
        if (!high_buffer || !low_buffer) {
            k_line->append(std::numeric_limits<double>::quiet_NaN());
            d_line->append(std::numeric_limits<double>::quiet_NaN());
            return;
        }
        
        auto high_array = high_buffer->array();
        auto low_array = low_buffer->array();
        
        // In streaming mode, find highest/lowest in last 'period' values
        double highest = -std::numeric_limits<double>::max();
        double lowest = std::numeric_limits<double>::max();
        
        // Calculate range: from (current_idx - period + 1) to current_idx
        int start_idx = current_idx - params.period + 1;
        int end_idx = current_idx;
        
        // Ensure start_idx is not negative
        if (start_idx < 0) start_idx = 0;
        
        // With initial NaN, skip index 0 if it's in our range
        if (has_initial_nan && start_idx == 0) {
            start_idx = 1;
        }
        
        for (int i = start_idx; i <= end_idx; ++i) {
            if (i >= 0 && i < static_cast<int>(high_array.size()) && i < static_cast<int>(low_array.size())) {
                if (!std::isnan(high_array[i]) && high_array[i] > highest) {
                    highest = high_array[i];
                }
                if (!std::isnan(low_array[i]) && low_array[i] < lowest) {
                    lowest = low_array[i];
                }
            }
        }
        
        if (highest == -std::numeric_limits<double>::max()) {
            highest = std::numeric_limits<double>::quiet_NaN();
        }
        if (lowest == std::numeric_limits<double>::max()) {
            lowest = std::numeric_limits<double>::quiet_NaN();
        }
        
        
        // Get close value at current position
        double close_val = std::numeric_limits<double>::quiet_NaN();
        auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
        if (close_buffer) {
            auto close_array = close_buffer->array();
            // Use end_idx which is the actual current position in the array
            if (end_idx >= 0 && end_idx < static_cast<int>(close_array.size())) {
                close_val = close_array[end_idx];
            }
        }
        
        double raw_k = 0.0;
        if (highest != lowest && !std::isnan(close_val)) {
            raw_k = 100.0 * (close_val - lowest) / (highest - lowest);
        } else if (params.safediv) {
            raw_k = params.safezero;
        } else {
            raw_k = std::numeric_limits<double>::quiet_NaN();
        }
        
        
        // Store raw k value
        k_values_.push_back(raw_k);
        if (k_values_.size() > params.period_dfast) {
            k_values_.erase(k_values_.begin());
        }
        
        // Debug output disabled
        // if (debug_call_count <= 20) {
        // }
        
        // Calculate slow %K (smoothed) - need enough raw K values
        double slow_k = std::numeric_limits<double>::quiet_NaN();
        
        
        // Calculate slow_k if we have enough raw K values
        // For slow %K, we need at least period_dfast raw K values
        // Which means we need at least (period + period_dfast - 1) data points total
        // But we should wait for the full minimum period before outputting any values
        int slow_k_minperiod = params.period + params.period_dfast - 1;
        if (k_values_.size() >= params.period_dfast && calc_count >= slow_k_minperiod) {
            double sum = 0.0;
            for (double val : k_values_) {
                sum += val;
            }
            slow_k = sum / params.period_dfast;
            
            // Debug output disabled
            // if (debug_call_count <= 20 || (calc_count >= 15 && calc_count <= 20)) {
            //               << ", k_values_.size()=" << k_values_.size()
            // }
        } // else if (debug_call_count <= 20 || (calc_count >= 15 && calc_count <= 20)) {
            //           << ", k_values_.size()=" << k_values_.size()
            //           << ", need calc_count>=" << (params.period + params.period_dfast - 1) 
        // }
        
        
        // In streaming mode, check against full minimum period
        // The Stochastic indicator's minimum period considers both %K and %D
        int full_minperiod = params.period + params.period_dfast + params.period_dslow - 2;
        
        // Debug disabled
        // std::cout << "Stochastic::calculate_lines DEBUG: current_data_size=" << current_data_size
        //           << ", calc_count=" << calc_count 
        //           << ", full_minperiod=" << full_minperiod
        //           << ", slow_k=" << slow_k
        //           << ", k_values_.size()=" << k_values_.size()
        //           << ", condition(calc_count<full_minperiod)=" << (calc_count < full_minperiod)
        //           << ", appending " << (calc_count < full_minperiod ? "NaN" : "value") << std::endl;
        
        if (calc_count < full_minperiod) {
            // Before minimum period, output NaN for both lines
            k_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // After minimum period, append calculated value
            k_line->append(slow_k);
        }
        
        // Debug disabled
        
        // Store slow k for %D calculation
        if (!std::isnan(slow_k)) {
            d_values_.push_back(slow_k);
            if (d_values_.size() > params.period_dslow) {
                d_values_.erase(d_values_.begin());
            }
        }
        
        // Calculate %D - need enough slow K values 
        double d_value = std::numeric_limits<double>::quiet_NaN();
        if (d_values_.size() >= params.period_dslow && calc_count >= d_min_period) {
            double sum = 0.0;
            for (double val : d_values_) {
                sum += val;
            }
            d_value = sum / params.period_dslow;
        }
        
        // In streaming mode, check against full minimum period  
        if (calc_count < full_minperiod) {
            // Before minimum period, output NaN
            d_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // After minimum period, append calculated value
            d_line->append(d_value);
        }
        
        return;
    }
    
    // Fallback to batch calculation for other cases
    
    if (lines->getline(percK)->size() == 0) {
        if (datas.empty() || !datas[0] || !datas[0]->lines) return;
        
        auto first_line = datas[0]->lines->getline(0);
        if (!first_line) return;
        
        int data_size = 0;
        auto buffer = std::dynamic_pointer_cast<LineBuffer>(first_line);
        if (buffer) {
            if (buffer->size() == 0) {
                data_size = static_cast<int>(buffer->array().size());
            } else {
                data_size = static_cast<int>(buffer->size());
            }
        } else {
            data_size = static_cast<int>(first_line->size());
        }
        
        once(0, data_size);
    }
}

// StochasticFull implementation
StochasticFull::StochasticFull() : StochasticBase() {
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
}

StochasticFull::StochasticFull(std::shared_ptr<LineSeries> data_source) 
    : StochasticBase() {
    setup_lines();
    
    if (data_source) {
        datas.push_back(data_source);
    }
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    // For StochasticFull: %K needs period, %D needs period + period_dfast - 1, 
    // %DSlow needs period + period_dfast + period_dslow - 2
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
}

StochasticFull::StochasticFull(std::shared_ptr<DataSeries> data_source)
    : StochasticBase() {
    setup_lines();
    
    // DataSeries has OHLCV data
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
        
            // Data integrity checks can be added here if needed
    }
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    // For StochasticFull: %K needs period, %D needs period + period_dfast - 1, 
    // %DSlow needs period + period_dfast + period_dslow - 2
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
}

StochasticFull::StochasticFull(std::shared_ptr<DataSeries> data_source, int period, int period_dfast, int period_dslow)
    : StochasticBase() {
    params.period = period;
    params.period_dfast = period_dfast;
    params.period_dslow = period_dslow;
    
    setup_lines();
    
    // DataSeries has OHLCV data
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    // For StochasticFull: %K needs period, %D needs period + period_dfast - 1, 
    // %DSlow needs period + period_dfast + period_dslow - 2
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
}

void StochasticFull::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void StochasticFull::calculate() {
    // Process all data in batch mode for test framework
    if (datas.empty() || !datas[0] || !datas[0]->lines) {
        return;
    }
    
    // Get data lines  
    // DataSeries structure: datetime=0, open=1, high=2, low=3, close=4, volume=5, openinterest=6
    auto high_line = datas[0]->lines->getline(2);   // High
    auto low_line = datas[0]->lines->getline(3);    // Low
    auto close_line = datas[0]->lines->getline(4);  // Close
    
    if (!high_line || !low_line || !close_line) {
        return;
    }
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    
    if (!high_buffer || !low_buffer || !close_buffer) return;
    
    // Get output lines
    auto k_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percK));
    auto d_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percD));
    auto dslow_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percDSlow));
    
    if (!k_line || !d_line || !dslow_line) return;
    
    // Get data arrays
    const auto& high_array = high_buffer->array();
    const auto& low_array = low_buffer->array();
    const auto& close_array = close_buffer->array();
    
    if (high_array.empty() || low_array.empty() || close_array.empty()) {
        return;
    }
    
    int data_size = static_cast<int>(high_array.size());
    
    // Skip initial NaN if present
    int start_offset = 0;
    if (data_size > 0 && std::isnan(high_array[0])) {
        start_offset = 1;
        data_size--;
    }
    
    // Reset output buffers for batch processing
    k_line->reset();
    d_line->reset();
    dslow_line->reset();
    
    // Add initial NaN if data had one
    if (start_offset > 0) {
        k_line->append(std::numeric_limits<double>::quiet_NaN());
        d_line->append(std::numeric_limits<double>::quiet_NaN());
        dslow_line->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Calculate raw %K values for all data points
    std::vector<double> raw_k_values;
    raw_k_values.reserve(data_size);
    
    for (int i = 0; i < data_size; ++i) {
        int idx = i + start_offset;  // Adjust for skipped NaN
        if (i < params.period - 1) {
            // Not enough data for calculation
            raw_k_values.push_back(std::numeric_limits<double>::quiet_NaN());
            k_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Calculate highest high and lowest low over the period
            double highest = -std::numeric_limits<double>::max();
            double lowest = std::numeric_limits<double>::max();
            
            for (int j = idx - params.period + 1; j <= idx; ++j) {
                if (!std::isnan(high_array[j]) && high_array[j] > highest) {
                    highest = high_array[j];
                }
                if (!std::isnan(low_array[j]) && low_array[j] < lowest) {
                    lowest = low_array[j];
                }
            }
            
            double current_close = close_array[idx];
            double raw_k = std::numeric_limits<double>::quiet_NaN();
            
            if (highest != lowest && !std::isnan(current_close)) {
                raw_k = 100.0 * (current_close - lowest) / (highest - lowest);
            } else if (params.safediv) {
                raw_k = params.safezero;
            }
            
            raw_k_values.push_back(raw_k);
            k_line->append(raw_k);
        }
    }
    
    // Calculate %D (SMA of %K)
    for (int i = 0; i < data_size; ++i) {
        if (i < params.period + params.period_dfast - 2) {
            d_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            double sum = 0.0;
            int count = 0;
            for (int j = i - params.period_dfast + 1; j <= i; ++j) {
                if (!std::isnan(raw_k_values[j])) {
                    sum += raw_k_values[j];
                    count++;
                }
            }
            double d_value = (count >= params.period_dfast) ? (sum / params.period_dfast) : std::numeric_limits<double>::quiet_NaN();
            d_line->append(d_value);
        }
    }
    
    // Calculate %DSlow (SMA of %D)
    const auto& d_array = d_line->array();
    for (int i = 0; i < data_size; ++i) {
        if (i < params.period + params.period_dfast + params.period_dslow - 3) {
            dslow_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            double sum = 0.0;
            int count = 0;
            for (int j = i - params.period_dslow + 1; j <= i; ++j) {
                if (!std::isnan(d_array[j])) {
                    sum += d_array[j];
                    count++;
                }
            }
            double dslow_value = (count >= params.period_dslow) ? (sum / params.period_dslow) : std::numeric_limits<double>::quiet_NaN();
            dslow_line->append(dslow_value);
        }
    }
}

void StochasticFull::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get data lines - for DataSeries, use OHLC indices
    std::shared_ptr<LineRoot> high_line;
    std::shared_ptr<LineRoot> low_line;
    std::shared_ptr<LineRoot> close_line;
    
    if (datas.size() >= 3) {
        // If we have multiple data sources (test framework pattern)
        high_line = datas[0]->lines->getline(0);
        low_line = datas[1]->lines->getline(0);
        close_line = datas[2]->lines->getline(0);
    } else {
        // Single data source with OHLC data
        // DataSeries structure: datetime=0, open=1, high=2, low=3, close=4, volume=5, openinterest=6
        high_line = datas[0]->lines->getline(2);   // High
        low_line = datas[0]->lines->getline(3);    // Low  
        close_line = datas[0]->lines->getline(4);  // Close
    }
    
    if (!high_line || !low_line || !close_line) return;
    
    auto k_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percK));
    auto d_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percD));
    auto dslow_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percDSlow));
    
    if (!k_line || !d_line || !dslow_line) return;
    
    // Get buffers for calculation
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    
    if (!high_buffer || !low_buffer || !close_buffer) return;
    
    // Determine current position
    auto high_array = high_buffer->array();
    auto low_array = low_buffer->array();
    auto close_array = close_buffer->array();
    
    if (high_array.empty() || low_array.empty() || close_array.empty()) return;
    
    int current_idx = static_cast<int>(high_array.size()) - 1;
    
    // Check minimum period
    if (current_idx < params.period - 1) {
        // Not enough data
        k_line->append(std::numeric_limits<double>::quiet_NaN());
        d_line->append(std::numeric_limits<double>::quiet_NaN());
        dslow_line->append(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate raw %K
    double highest = -std::numeric_limits<double>::max();
    double lowest = std::numeric_limits<double>::max();
    
    // Find highest/lowest in last 'period' values
    int start_idx = current_idx - params.period + 1;
    if (start_idx < 0) start_idx = 0;
    
    for (int i = start_idx; i <= current_idx; ++i) {
        if (i >= 0 && i < static_cast<int>(high_array.size()) && i < static_cast<int>(low_array.size())) {
            if (!std::isnan(high_array[i]) && high_array[i] > highest) {
                highest = high_array[i];
            }
            if (!std::isnan(low_array[i]) && low_array[i] < lowest) {
                lowest = low_array[i];
            }
        }
    }
    
    double current_close = close_array[current_idx];
    
    double raw_k = 0.0;
    if (highest != lowest && !std::isnan(current_close)) {
        raw_k = 100.0 * (current_close - lowest) / (highest - lowest);
    } else if (params.safediv) {
        raw_k = params.safezero;
    } else {
        raw_k = std::numeric_limits<double>::quiet_NaN();
    }
    
    // Store raw k value
    k_values_.push_back(raw_k);
    if (k_values_.size() > params.period_dfast) {
        k_values_.erase(k_values_.begin());
    }
    
    // Calculate slow %K (smoothed) - this is what we output as %K
    double slow_k = std::numeric_limits<double>::quiet_NaN();
    if (k_values_.size() >= params.period_dfast && current_idx >= params.period + params.period_dfast - 2) {
        double sum = 0.0;
        for (double val : k_values_) {
            sum += val;
        }
        slow_k = sum / params.period_dfast;
    }
    
    k_line->append(slow_k);
    
    // Store slow k for %D calculation
    if (!std::isnan(slow_k)) {
        d_values_.push_back(slow_k);
        if (d_values_.size() > params.period_dslow) {
            d_values_.erase(d_values_.begin());
        }
    }
    
    // Calculate %D - SMA of slow %K
    double d_value = std::numeric_limits<double>::quiet_NaN();
    if (d_values_.size() >= params.period_dslow && current_idx >= params.period + params.period_dfast + params.period_dslow - 3) {
        double sum = 0.0;
        for (double val : d_values_) {
            sum += val;
        }
        d_value = sum / params.period_dslow;
    }
    
    d_line->append(d_value);
    
    // Store d value for %DSlow calculation (for StochasticFull)
    std::vector<double> dslow_values;
    // Get all d_line values for dslow calculation
    const auto& d_array = d_line->array();
    int d_size = static_cast<int>(d_array.size());
    
    // Calculate %DSlow - SMA of %D
    double dslow_value = std::numeric_limits<double>::quiet_NaN();
    if (d_size >= params.period_dslow && current_idx >= params.period + params.period_dfast + params.period_dslow - 2) {
        double sum = 0.0;
        int count = 0;
        for (int i = d_size - params.period_dslow; i < d_size; ++i) {
            if (i >= 0 && !std::isnan(d_array[i])) {
                sum += d_array[i];
                count++;
            }
        }
        if (count >= params.period_dslow) {
            dslow_value = sum / params.period_dslow;
        }
    }
    
    dslow_line->append(dslow_value);
}

void Stochastic::next() {
    // For streaming mode - calculate single value
    calculate_lines();
}

void Stochastic::calculate() {
    // Debug to see if this is being called
    static int calc_count = 0;
    calc_count++;
    // Debug output disabled
    // std::cout << "Stochastic::calculate() called, count=" << calc_count 
    //           << ", datas.size()=" << datas.size() << std::endl;
    
    // For test framework with separate data sources (high, low, close)
    if (datas.size() >= 3) {
        // Check if we have output lines
        auto k_line = lines->getline(percK);
        auto d_line = lines->getline(percD);
        
        if (!k_line || !d_line) {
            return;
        }
        
        auto k_buffer = std::dynamic_pointer_cast<LineBuffer>(k_line);
        auto d_buffer = std::dynamic_pointer_cast<LineBuffer>(d_line);
        
        if (!k_buffer || !d_buffer) {
            return;
        }
        
        // Check if we're in streaming mode by looking at buffer size
        // In streaming mode, the buffer size will be less than or equal to the input data size
        // In batch mode, we would have already calculated all values
        
        // datas[0] could be either a DataSeries with lines member, or a LineSeries directly
        std::shared_ptr<LineBuffer> high_buffer;
        if (datas[0]->lines && datas[0]->lines->size() > 0) {
            // DataSeries case
            auto high_line = datas[0]->lines->getline(0);
            high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
        } else {
            // LineSeries case - try to cast datas[0] directly
            auto line_series = std::dynamic_pointer_cast<LineSeries>(datas[0]);
            if (line_series && line_series->lines && line_series->lines->size() > 0) {
                auto high_line = line_series->lines->getline(0);
                high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
            }
        }
        
        if (!high_buffer) return;
        
        // Detect streaming mode: when we need to calculate one value at a time
        // vs batch mode: when all data is available upfront
        // In streaming mode, k_buffer grows incrementally with each calculate() call
        bool is_streaming_mode = true;  // Default to streaming
        
        // Check if this looks like batch mode: 
        // - high_buffer has much more data than k_buffer (indicating pre-loaded data)
        // - AND k_buffer has very few values (indicating we haven't started calculating)
        if (high_buffer->array().size() > params.period * 2 && k_buffer->array().size() <= 1) {
            // This is likely batch mode - all data loaded at once
            is_streaming_mode = false;
        }
        // In streaming mode, k_buffer size will grow with each call
        // It may eventually equal high_buffer size, but that doesn't mean we switch to batch mode
        
        // Debug for understanding the issue (disabled)
        // static int calc_debug = 0;
        // calc_debug++;
        // if (calc_debug <= 25) {
        //               << ", high_buffer->size()=" << high_buffer->size()
        // }
        
        if (is_streaming_mode) {
            // In streaming mode, track data by checking input buffer
            // high_buffer already obtained above
            
            // In streaming mode, k_buffer size tells us how many times calculate has been called
            // Each call should calculate one new value
            // The input data size is what matters for minimum period check
            const auto& arr = high_buffer->array();
            int input_data_size = static_cast<int>(arr.size());
            
            // Adjust for initial NaN if present
            bool has_initial_nan = (input_data_size > 0 && std::isnan(arr[0]));
            int actual_data_count = has_initial_nan ? input_data_size - 1 : input_data_size;
            
            // Debug disabled
            // if (input_data_size >= 15 && input_data_size <= 20) {
            //     std::cout << "Stochastic streaming mode: input_data_size=" << input_data_size
            //               << ", actual_data_count=" << actual_data_count << std::endl;
            // }
            
            // Debug (disabled)
            // static int debug_count = 0;
            // if (debug_count++ < 20) {
            //     std::cout << "Stochastic streaming #" << debug_count 
            //               << ": actual_data_count=" << actual_data_count 
            //               << ", minperiod=" << getMinPeriod() 
            //               << ", k_buffer size=" << k_buffer->size()
            // }
            
            // Always call calculate_lines() in streaming mode
            // Let calculate_lines() handle the minimum period check internally
            calculate_lines();
            return;
        }
        
        // Batch mode - check if already calculated
        // high_buffer already obtained above
        if (!is_streaming_mode && high_buffer && high_buffer->array().size() > params.period) {
            // Debug
            int input_data_size = static_cast<int>(high_buffer->array().size());
            if (input_data_size >= 15 && input_data_size <= 20) {
            }
            if (k_buffer->array().size() <= 1) {  // Only initial NaN
                calculate_with_separate_lines();
            }
            return;
        }
        
        // Fallback - this shouldn't happen
        // if (calc_debug <= 25) {
        // }
        return;
    }
    
    // Single datasource mode (normal usage)
    if (data && data->lines && data->lines->size() > 0) {
        calculate_with_single_datasource();
        return;
    }
    
    // Fallback to base class behavior
    StochasticBase::calculate();
}

void Stochastic::calculate_with_separate_lines() {
    // Debug
    
    // This method should not be used in streaming mode
    // Streaming mode should use the calculate_lines() method which handles single values
    // This method is only for batch calculation
    if (datas.size() < 3) {
        return;
    }
    
    auto high_line = datas[0]->lines->getline(0);
    auto low_line = datas[1]->lines->getline(0);
    auto close_line = datas[2]->lines->getline(0);
    
    if (!high_line || !low_line || !close_line) {
        return;
    }
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    
    if (!high_buffer || !low_buffer || !close_buffer) {
        return;
    }
    
    auto high_array = high_buffer->array();
    auto low_array = low_buffer->array();
    auto close_array = close_buffer->array();
    
    if (high_array.empty() || low_array.empty() || close_array.empty()) {
        return;
    }
    
    int data_size = static_cast<int>(high_array.size());
    calculate_stochastic_values(high_array, low_array, close_array, data_size);
}

void Stochastic::calculate_with_single_datasource() {
    // DataSeries structure: datetime=0, open=1, high=2, low=3, close=4, volume=5, openinterest=6
    auto high_line = data->lines->getline(2);  // High
    auto low_line = data->lines->getline(3);   // Low
    auto close_line = data->lines->getline(4); // Close
    
    if (!high_line || !low_line || !close_line) return;
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    
    if (!high_buffer || !low_buffer || !close_buffer) return;
    
    auto high_array = high_buffer->array();
    auto low_array = low_buffer->array();
    auto close_array = close_buffer->array();
    
    if (high_array.empty() || low_array.empty() || close_array.empty()) return;
    
    int data_size = static_cast<int>(high_array.size());
    calculate_stochastic_values(high_array, low_array, close_array, data_size);
}

void Stochastic::calculate_stochastic_values(const std::vector<double>& high_array, 
                                           const std::vector<double>& low_array,
                                           const std::vector<double>& close_array,
                                           int data_size) {
    auto k_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percK));
    auto d_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percD));
    
    if (!k_line || !d_line) {
        return;
    }
    
    // Reset buffers
    k_line->reset();
    d_line->reset();
    
    // The minimum period for Stochastic (slow) is:
    // period + period_dfast + period_dslow - 2
    // With defaults: 14 + 3 + 3 - 2 = 18
    // But Python seems to use 18 as the minperiod
    int min_period = params.period + params.period_dfast + params.period_dslow - 2;
    
    // Step 1: Calculate raw %K values for each data point
    std::vector<double> raw_k_values;
    
    for (int i = 0; i < data_size; ++i) {
        if (i < params.period - 1) {
            // Not enough data for calculation
            raw_k_values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Calculate highest high and lowest low over the period
            double highest = -std::numeric_limits<double>::max();
            double lowest = std::numeric_limits<double>::max();
            
            for (int j = 0; j < params.period; ++j) {
                int idx = i - j;
                if (idx >= 0 && idx < static_cast<int>(high_array.size())) {
                    if (!std::isnan(high_array[idx]) && high_array[idx] > highest) {
                        highest = high_array[idx];
                    }
                    if (!std::isnan(low_array[idx]) && low_array[idx] < lowest) {
                        lowest = low_array[idx];
                    }
                }
            }
            
            double current_close = (i < static_cast<int>(close_array.size())) ? close_array[i] : std::numeric_limits<double>::quiet_NaN();
            
            double raw_k = 0.0;
            if (highest != lowest && !std::isnan(current_close)) {
                raw_k = 100.0 * (current_close - lowest) / (highest - lowest);
            } else if (params.safediv) {
                raw_k = params.safezero;
            } else {
                raw_k = std::numeric_limits<double>::quiet_NaN();
            }
            
            raw_k_values.push_back(raw_k);
        }
    }
    
    // Step 2: Calculate Slow %K (SMA of raw %K) - this becomes our %K line
    std::vector<double> slow_k_values;
    
    for (int i = 0; i < data_size; ++i) {
        // The slow %K line needs:
        // - At least 'period' bars to calculate raw %K (index >= period - 1)
        // - Then 'period_dfast' bars to smooth it (index >= period - 1 + period_dfast - 1 = period + period_dfast - 2)
        // So slow %K first valid value is at index period + period_dfast - 2
        
        if (i < params.period - 1 + params.period_dfast - 1) {
            // Not enough data for slow %K calculation
            k_line->append(std::numeric_limits<double>::quiet_NaN());
            slow_k_values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Calculate SMA of raw %K values over period_dfast
            double sum = 0.0;
            int valid_count = 0;
            
            for (int j = 0; j < params.period_dfast; ++j) {
                int k_idx = i - j;
                if (k_idx >= 0 && k_idx < static_cast<int>(raw_k_values.size()) && !std::isnan(raw_k_values[k_idx])) {
                    sum += raw_k_values[k_idx];
                    valid_count++;
                }
            }
            
            double slow_k = (valid_count >= params.period_dfast) ? (sum / params.period_dfast) : std::numeric_limits<double>::quiet_NaN();
            k_line->append(slow_k);
            slow_k_values.push_back(slow_k);
        }
    }
    
    // Step 3: Calculate %D (SMA of Slow %K)
    for (int i = 0; i < data_size; ++i) {
        // The %D line needs slow %K values to be valid
        // %D can start as soon as we have period_dslow valid slow %K values
        // Slow %K starts at index period + period_dfast - 2
        // %D needs period_dslow slow %K values, so %D starts at index (period + period_dfast - 2) + period_dslow - 1
        // With period=14, period_dfast=3, period_dslow=3: (14 + 3 - 2) + 3 - 1 = 15 + 2 = 17
        // So index 17 is the first valid %D value (minperiod-1), index 18 is the second valid value
        int d_min_period = params.period - 1 + params.period_dfast - 1 + params.period_dslow - 1;
        
        if (i < d_min_period) {
            // Not enough data for %D calculation
            d_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Calculate SMA of slow %K values over period_dslow
            double sum = 0.0;
            int valid_count = 0;
            
            for (int j = 0; j < params.period_dslow; ++j) {
                int k_idx = i - j;
                if (k_idx >= 0 && k_idx < static_cast<int>(slow_k_values.size()) && !std::isnan(slow_k_values[k_idx])) {
                    sum += slow_k_values[k_idx];
                    valid_count++;
                }
            }
            
            double d_value = (valid_count >= params.period_dslow) ? (sum / params.period_dslow) : std::numeric_limits<double>::quiet_NaN();
            d_line->append(d_value);
        }
    }
    
    
    // After batch processing, position LineBuffer to use the most recent calculated values
    // For batch mode, we want to access the latest calculated values
    if (!k_line->array().empty()) {
        // Position to the last calculated value
        k_line->set_idx(k_line->array().size() - 1);
    }
    if (!d_line->array().empty()) {
        // Position to the last calculated value  
        d_line->set_idx(d_line->array().size() - 1);
    }
}

void Stochastic::once(int start, int end) {
    // For test framework constructor with separate high/low/close data sources
    if (datas.size() >= 3) {
        // Multi-datasource mode (test framework)
        calculate_with_separate_lines();
        return;
    }
    
    // Single datasource mode (normal usage)
    if (data && data->lines && data->lines->size() > 0) {
        calculate_with_single_datasource();
        return;
    }
    
    // Error: Neither datas nor data available
}

// StochasticBase utility methods for test framework compatibility
double StochasticBase::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto k_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percK));
    if (!k_buffer) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use LineBuffer's get() method which should handle indexing correctly
    return k_buffer->get(ago);
}

double StochasticBase::get_d(int ago) const {
    if (!lines || lines->size() < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto d_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(percD));
    if (!d_buffer) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use LineBuffer's get() method which should handle indexing correctly
    return d_buffer->get(ago);
}

size_t StochasticBase::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto percK_line = lines->getline(percK);
    if (!percK_line) {
        return 0;
    }
    
    // Get the actual LineBuffer to check for initial NaN
    auto k_buffer = std::dynamic_pointer_cast<LineBuffer>(percK_line);
    if (k_buffer) {
        // If the buffer has an initial NaN, return size-1 to match Python behavior
        const auto& arr = k_buffer->array();
        if (!arr.empty() && std::isnan(arr[0])) {
            // Has initial NaN, so effective size is array size - 1
            return arr.size() > 0 ? arr.size() - 1 : 0;
        }
        // No initial NaN or not LineBuffer
        return k_buffer->size();
    }
    
    // Fallback for non-LineBuffer
    return percK_line->size();
}


int StochasticBase::getMinPeriod() const {
    int minperiod_value = _minperiod();
    return minperiod_value;
}

double StochasticBase::getPercentK(int ago) const {
    return get(ago);
}

double StochasticBase::getPercentD(int ago) const {
    return get_d(ago);
}

} // namespace backtrader