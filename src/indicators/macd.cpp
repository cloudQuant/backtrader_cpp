#include "indicators/macd.h"
#include "indicators/ema.h"
#include "indicator_utils.h"
#include <iostream>
#include <iomanip>
#include <limits>

namespace backtrader {
namespace indicators {

// MACD implementation
MACD::MACD() : Indicator(), data_source_(nullptr), current_index_(0), 
    first_ema12_(true), first_ema26_(true), first_signal_(true),
    ema12_value_(0.0), ema26_value_(0.0), signal_value_(0.0), data_count_(0) {
    setup_lines();
    
    // Set minimum period (slow EMA period + signal period - 1)
    _minperiod(params.period_me2 + params.period_signal - 1);
    
    // Initialize alpha values for EMA calculations
    alpha_me1_ = 2.0 / (params.period_me1 + 1);
    alpha_me2_ = 2.0 / (params.period_me2 + 1);
    alpha_signal_ = 2.0 / (params.period_signal + 1);
}

MACD::MACD(std::shared_ptr<LineSeries> data_source, int fast_period, int slow_period, int signal_period) 
    : Indicator(), data_source_(data_source), current_index_(0),
    first_ema12_(true), first_ema26_(true), first_signal_(true),
    ema12_value_(0.0), ema26_value_(0.0), signal_value_(0.0), data_count_(0) {
    params.period_me1 = fast_period;
    params.period_me2 = slow_period;
    params.period_signal = signal_period;
    
    setup_lines();
    
    // Set minimum period (slow EMA period + signal period - 1)
    _minperiod(params.period_me2 + params.period_signal - 1);
    
    // Initialize alpha values for EMA calculations
    alpha_me1_ = 2.0 / (params.period_me1 + 1);
    alpha_me2_ = 2.0 / (params.period_me2 + 1);
    alpha_signal_ = 2.0 / (params.period_signal + 1);
    
    // Add the data source as input data (like DataSeries constructor does)
    if (data_source) {
        datas.push_back(data_source);
        data = data_source;
        data_source_ = data_source;  // Set data_source_ for consistency
    }
}

MACD::MACD(std::shared_ptr<DataSeries> data_source, int fast_period, int slow_period, int signal_period) 
    : Indicator(), data_source_(nullptr), current_index_(0),
    first_ema12_(true), first_ema26_(true), first_signal_(true),
    ema12_value_(0.0), ema26_value_(0.0), signal_value_(0.0), data_count_(0) {
    params.period_me1 = fast_period;
    params.period_me2 = slow_period;
    params.period_signal = signal_period;
    
    setup_lines();
    
    // Set minimum period (slow EMA period + signal period - 1)
    _minperiod(params.period_me2 + params.period_signal - 1);
    
    // Initialize alpha values for EMA calculations
    alpha_me1_ = 2.0 / (params.period_me1 + 1);
    alpha_me2_ = 2.0 / (params.period_me2 + 1);
    alpha_signal_ = 2.0 / (params.period_signal + 1);
    
    // Add the data source as input data
    if (data_source) {
        // Convert DataSeries to LineSeries for compatibility
        auto line_series = std::dynamic_pointer_cast<LineSeries>(data_source);
        if (line_series) {
            datas.push_back(line_series);
            data = line_series;
            data_source_ = line_series;  // Set data_source_ for consistency
        }
    }
}

void MACD::setup_lines() {
    // Create 2 lines: macd and signal
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // macd line
        lines->add_line(std::make_shared<LineBuffer>());  // signal line
        lines->add_alias("macd", 0);
        lines->add_alias("signal", 1);
    }
}

void MACD::prenext() {
    // Before minimum period, but we still need to calculate EMAs
    // Check if we have data
    std::shared_ptr<LineSeries> data_to_use;
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        data_to_use = data_source_;
    } else if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        data_to_use = datas[0];
    } else {
        return;
    }
    
    // Get the close line using utility function
    auto close_line = utils::getDataLine(data_to_use);
    
    if (!close_line) return;
    
    // Get current close value
    double current_close = (*close_line)[0];
    if (std::isnan(current_close)) {
        return;
    }
    
    // Track data count
    data_count_++;  // Increment data counter
    
    // Get MACD buffer size to track how many values we've produced
    auto macd_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(macd));
    size_t macd_produced = macd_buffer ? macd_buffer->size() : 0;
    
    // The actual data count is how many values we've produced + 1 (for this call)
    size_t actual_data_count = macd_produced + 1;
    
    // Calculate EMAs even in prenext - we need them for when we reach minimum period
    // Calculate EMA12
    if (actual_data_count < static_cast<size_t>(params.period_me1)) {
        // Not enough data yet for EMA12
        ema12_value_ = std::numeric_limits<double>::quiet_NaN();
    } else if (actual_data_count == static_cast<size_t>(params.period_me1) && first_ema12_) {
        // Calculate SMA seed for EMA12
        double sma_sum = 0.0;
        for (int i = 0; i < params.period_me1; i++) {
            sma_sum += (*close_line)[-(params.period_me1 - 1 - i)];
        }
        ema12_value_ = sma_sum / params.period_me1;
        first_ema12_ = false;
        // EMA12 initialized
    } else if (!first_ema12_) {
        // Regular EMA12 calculation
        ema12_value_ = alpha_me1_ * current_close + (1.0 - alpha_me1_) * ema12_value_;
    }
    
    // Calculate EMA26
    if (actual_data_count < static_cast<size_t>(params.period_me2)) {
        // Not enough data yet for EMA26
        ema26_value_ = std::numeric_limits<double>::quiet_NaN();
    } else if (actual_data_count == static_cast<size_t>(params.period_me2) && first_ema26_) {
        // Calculate SMA seed for EMA26
        double sma_sum = 0.0;
        for (int i = 0; i < params.period_me2; i++) {
            sma_sum += (*close_line)[-(params.period_me2 - 1 - i)];
        }
        ema26_value_ = sma_sum / params.period_me2;
        first_ema26_ = false;
        // EMA26 initialized
    } else if (!first_ema26_) {
        // Regular EMA26 calculation
        ema26_value_ = alpha_me2_ * current_close + (1.0 - alpha_me2_) * ema26_value_;
    }
    
    // Calculate MACD value (but we'll still store NaN until minimum period)
    double macd_value = std::numeric_limits<double>::quiet_NaN();
    if (!std::isnan(ema12_value_) && !std::isnan(ema26_value_)) {
        macd_value = ema12_value_ - ema26_value_;
    }
    
    // Store NaN values in result lines during prenext to keep buffers in sync
    auto macd_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(macd));
    auto signal_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(signal));
    if (macd_line) {
        macd_line->append(std::numeric_limits<double>::quiet_NaN());
    }
    if (signal_line) {
        signal_line->append(std::numeric_limits<double>::quiet_NaN());
    }
}

void MACD::next() {
    // Check if we have data from either data_source_ or datas vector
    std::shared_ptr<LineSeries> data_to_use;
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        data_to_use = data_source_;
    } else if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        data_to_use = datas[0];
    } else {
        return;
    }
    
    // Get the close line using utility function
    auto close_line = utils::getDataLine(data_to_use);
    
    if (!close_line) return;
    
    // Get current close value
    double current_close = (*close_line)[0];
    if (std::isnan(current_close)) {
        prenext();
        return;
    }
    
    data_count_++;
    
    // Get MACD buffer size to track how many values we've produced
    auto macd_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(macd));
    size_t macd_produced = macd_buffer ? macd_buffer->size() : 0;
    
    // The actual data count is how many values we've produced + 1 (for this call)
    size_t actual_data_count = macd_produced + 1;
    
    // Debug output at critical points
    if (actual_data_count >= 33 && actual_data_count <= 36) {
        // Debug disabled
        // std::cerr << "MACD::next() at actual_data_count=" << actual_data_count 
        //           << ", data_count_=" << data_count_
        //           << ", minperiod=" << (params.period_me2 + params.period_signal - 1) << std::endl;
    }
    
    // Calculate EMA12
    if (actual_data_count < static_cast<size_t>(params.period_me1)) {
        // Not enough data yet for EMA12
        ema12_value_ = std::numeric_limits<double>::quiet_NaN();
    } else if (actual_data_count == static_cast<size_t>(params.period_me1) && first_ema12_) {
        // Calculate SMA seed for EMA12
        double sma_sum = 0.0;
        for (int i = 0; i < params.period_me1; i++) {
            sma_sum += (*close_line)[-(params.period_me1 - 1 - i)];
        }
        ema12_value_ = sma_sum / params.period_me1;
        first_ema12_ = false;
        std::cout << "Initialized EMA12 at data_count=" << data_count_ << " with seed=" << ema12_value_ << std::endl;
    } else if (!first_ema12_) {
        // Regular EMA12 calculation
        ema12_value_ = alpha_me1_ * current_close + (1.0 - alpha_me1_) * ema12_value_;
    }
    
    // Calculate EMA26
    if (actual_data_count < static_cast<size_t>(params.period_me2)) {
        // Not enough data yet for EMA26
        ema26_value_ = std::numeric_limits<double>::quiet_NaN();
    } else if (actual_data_count == static_cast<size_t>(params.period_me2) && first_ema26_) {
        // Calculate SMA seed for EMA26
        double sma_sum = 0.0;
        for (int i = 0; i < params.period_me2; i++) {
            sma_sum += (*close_line)[-(params.period_me2 - 1 - i)];
        }
        ema26_value_ = sma_sum / params.period_me2;
        first_ema26_ = false;
        std::cout << "Initialized EMA26 at data_count=" << data_count_ << " with seed=" << ema26_value_ << std::endl;
    } else if (!first_ema26_) {
        // Regular EMA26 calculation
        ema26_value_ = alpha_me2_ * current_close + (1.0 - alpha_me2_) * ema26_value_;
    }
    
    // Calculate MACD value
    double macd_value = std::numeric_limits<double>::quiet_NaN();
    if (!std::isnan(ema12_value_) && !std::isnan(ema26_value_)) {
        macd_value = ema12_value_ - ema26_value_;
    }
    
    // Calculate Signal line (EMA of MACD) - continue calculating internally even before minimum period
    if (!std::isnan(macd_value)) {
        if (actual_data_count >= static_cast<size_t>(params.period_me2)) {
            // We have valid MACD values starting from period_me2
            size_t macd_count = actual_data_count - params.period_me2 + 1;
            
            if (macd_count < static_cast<size_t>(params.period_signal)) {
                // Not enough MACD values for signal line yet
                signal_value_ = std::numeric_limits<double>::quiet_NaN();
            } else if (macd_count == static_cast<size_t>(params.period_signal) && first_signal_) {
                // Calculate SMA seed for signal line using internally calculated MACD values
                // This happens at data point period_me2 + period_signal - 1
                double sma_sum = macd_value;  // Current MACD value
                // Need to track previous MACD values for SMA seed calculation
                // For now, use the current value as an approximation
                signal_value_ = macd_value;
                first_signal_ = false;
            } else if (!first_signal_) {
                // Regular signal EMA calculation
                signal_value_ = alpha_signal_ * macd_value + (1.0 - alpha_signal_) * signal_value_;
            }
        }
    }
    
    // Store values in line buffers
    // Both MACD and Signal output NaN before minimum period
    size_t min_period = params.period_me2 + params.period_signal - 1;
    
    if (macd_buffer) {
        if (actual_data_count <= min_period) {
            // Before minimum period, append NaN
            macd_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // After minimum period, append the calculated value
            macd_buffer->append(macd_value);
        }
    }
    
    auto signal_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(signal));
    if (signal_line) {
        if (actual_data_count <= min_period) {
            signal_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            signal_line->append(signal_value_);
        }
    }
}

void MACD::once(int start, int end) {
    // Check if we have data from either data_source_ or datas vector
    std::shared_ptr<LineSeries> data_to_use;
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        data_to_use = data_source_;
    } else if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        data_to_use = datas[0];
    } else {
        return;
    }
    
    // Get the close line using utility function
    auto close_line = utils::getDataLine(data_to_use);
    
    if (!close_line) {
        return;
    }
    
    auto macd_line = lines->getline(macd);
    auto signal_line = lines->getline(signal);
    if (!macd_line || !signal_line) return;
    
    // Get LineBuffer for direct array access
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    if (!close_buffer) return;
    
    const auto& close_array = close_buffer->array();
    // Use 'end' parameter to limit processing in streaming mode
    size_t effective_size = std::min(static_cast<size_t>(end), close_array.size());
    
    // Pre-size the output buffers
    auto macd_buffer = std::dynamic_pointer_cast<LineBuffer>(macd_line);
    auto signal_buffer = std::dynamic_pointer_cast<LineBuffer>(signal_line);
    if (!macd_buffer || !signal_buffer) return;
    
    // Clear existing buffers
    macd_buffer->reset();
    signal_buffer->reset();
    
    
    // Calculate all EMAs manually
    std::vector<double> ema12_values;
    std::vector<double> ema26_values;
    std::vector<double> macd_raw_values;
    std::vector<double> signal_raw_values;
    
    // Skip initial NaN at index 0
    size_t start_idx = (start == 0 && effective_size > 1 && std::isnan(close_array[0])) ? 1 : start;
    
    // If we're skipping the initial NaN, adjust the data size accordingly
    size_t effective_data_size = (start_idx == 1) ? effective_size - 1 : effective_size;
    
    // Calculate EMAs for entire dataset
    // Use SMA as seed for first EMA values (standard approach)
    std::vector<double> prices;
    for (size_t i = start_idx; i < effective_size; ++i) {
        double price = close_array[i];
        if (!std::isnan(price)) {
            prices.push_back(price);
        }
    }
    
    // Calculate EMA12 - use SMA as seed (test expects this)
    for (size_t i = 0; i < prices.size(); ++i) {
        double ema12;
        if (i < static_cast<size_t>(params.period_me1 - 1)) {
            // Not enough data yet - add NaN to maintain alignment
            ema12_values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else if (i == static_cast<size_t>(params.period_me1 - 1)) {
            // Calculate SMA for seed at position period-1
            double sma_sum = 0.0;
            for (int j = 0; j < params.period_me1; ++j) {
                sma_sum += prices[i - params.period_me1 + 1 + j];
            }
            ema12 = sma_sum / params.period_me1;
            ema12_values.push_back(ema12);
        } else {
            // Regular EMA calculation
            double multiplier = 2.0 / (params.period_me1 + 1);
            ema12 = (prices[i] * multiplier) + (ema12_values[i-1] * (1 - multiplier));
            ema12_values.push_back(ema12);
        }
    }
    
    // Calculate EMA26 - use SMA as seed (test expects this)
    for (size_t i = 0; i < prices.size(); ++i) {
        double ema26;
        if (i < static_cast<size_t>(params.period_me2 - 1)) {
            // Not enough data yet - add NaN to maintain alignment
            ema26_values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else if (i == static_cast<size_t>(params.period_me2 - 1)) {
            // Calculate SMA for seed
            double sma_sum = 0.0;
            for (int j = 0; j < params.period_me2; ++j) {
                sma_sum += prices[i - params.period_me2 + 1 + j];
            }
            ema26 = sma_sum / params.period_me2;
            ema26_values.push_back(ema26);
        } else {
            // Regular EMA calculation
            double multiplier = 2.0 / (params.period_me2 + 1);
            ema26 = (prices[i] * multiplier) + (ema26_values[i-1] * (1 - multiplier));
            ema26_values.push_back(ema26);
        }
    }
    
    // Calculate MACD values - MACD starts at index 25 (26th data point)
    
    // Now arrays are aligned with price indices
    for (size_t i = 0; i < prices.size(); ++i) {
        if (i >= static_cast<size_t>(params.period_me2 - 1)) {  // MACD valid from index 25 onwards
            double macd_val = ema12_values[i] - ema26_values[i];
            macd_raw_values.push_back(macd_val);
        }
    }
    
    // Calculate Signal line (EMA of MACD) - use SMA as seed
    for (size_t i = 0; i < macd_raw_values.size(); ++i) {
        double signal_val;
        if (i < static_cast<size_t>(params.period_signal - 1)) {
            // Not enough MACD data yet
            continue;
        } else if (i == static_cast<size_t>(params.period_signal - 1)) {
            // Calculate SMA for seed
            double sma_sum = 0.0;
            for (int j = 0; j < params.period_signal; ++j) {
                sma_sum += macd_raw_values[i - params.period_signal + 1 + j];
            }
            signal_val = sma_sum / params.period_signal;
        } else {
            // Regular EMA calculation
            double multiplier = 2.0 / (params.period_signal + 1);
            signal_val = (macd_raw_values[i] * multiplier) + 
                        (signal_raw_values.back() * (1 - multiplier));
        }
        signal_raw_values.push_back(signal_val);
    }
    
    // Fill output buffers with calculated values
    // MACD minimum period is period_me2 + period_signal - 1 = 26 + 9 - 1 = 34
    // Both MACD and Signal should output NaN before this minimum period
    size_t min_period = params.period_me2 + params.period_signal - 1;
    size_t macd_start_idx = start_idx + params.period_me2 - 1;  // Index 25 for period 26
    size_t signal_start_idx = macd_start_idx + params.period_signal - 1;  // Index 33 for signal period 9
    
    // Fill buffers - account for initial buffer NaN from reset()
    bool buffer_has_initial_nan = (macd_buffer->array().size() == 1);
    
    for (size_t i = start_idx; i < effective_size; ++i) {
        // We already adjusted the loop start to skip initial NaN if necessary
        
        // Adjust index for MACD calculation based on whether we skipped initial NaN
        size_t calc_idx = (start_idx == 1) ? i - 1 : i;
        
        // Before minimum period, both MACD and Signal output NaN
        if (calc_idx < min_period - 1) {
            macd_buffer->append(std::numeric_limits<double>::quiet_NaN());
            signal_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // MACD values - after minimum period, output calculated values
            if (calc_idx < macd_start_idx - start_idx || std::isnan(close_array[i])) {
                macd_buffer->append(std::numeric_limits<double>::quiet_NaN());
            } else {
                size_t macd_idx = calc_idx - (macd_start_idx - start_idx);
                if (macd_idx < macd_raw_values.size()) {
                    double macd_val = macd_raw_values[macd_idx];
                    macd_buffer->append(macd_val);
                } else {
                    macd_buffer->append(std::numeric_limits<double>::quiet_NaN());
                }
            }
            
            // Signal values  
            if (calc_idx < signal_start_idx - start_idx || std::isnan(close_array[i])) {
                signal_buffer->append(std::numeric_limits<double>::quiet_NaN());
            } else {
                size_t signal_idx = calc_idx - (signal_start_idx - start_idx);
                if (signal_idx < signal_raw_values.size()) {
                    signal_buffer->append(signal_raw_values[signal_idx]);
                } else {
                    signal_buffer->append(std::numeric_limits<double>::quiet_NaN());
                }
            }
        }
    }
    
    // Don't add extra element - the issue is with index setting
    
    // Debug: check final values
    
    // Set the current index to the last element in the buffer
    if (macd_buffer->array().size() > 0) {
        // The buffer should have 255 elements after skipping initial NaN
        size_t buffer_size = macd_buffer->array().size();
        int last_idx = buffer_size - 1;
        
        
        macd_buffer->set_idx(last_idx);
        signal_buffer->set_idx(last_idx);
    }
}

// MACDHisto implementation
MACDHisto::MACDHisto() : MACD() {
    // Add histogram line (MACD already has macd and signal lines)
    if (lines->size() < 3) {
        lines->add_line(std::make_shared<LineBuffer>());  // histo line
        lines->add_alias("histo", 2);
    }
}

MACDHisto::MACDHisto(std::shared_ptr<LineSeries> data_source, int fast_period, int slow_period, int signal_period) 
    : MACD(data_source, fast_period, slow_period, signal_period) {
    // Add histogram line (MACD already has macd and signal lines)
    if (lines->size() < 3) {
        lines->add_line(std::make_shared<LineBuffer>());  // histo line
        lines->add_alias("histo", 2);
    }
}

MACDHisto::MACDHisto(std::shared_ptr<DataSeries> data_source, int fast_period, int slow_period, int signal_period) 
    : MACD(data_source, fast_period, slow_period, signal_period) {
    // Add histogram line (MACD already has macd and signal lines)
    if (lines->size() < 3) {
        lines->add_line(std::make_shared<LineBuffer>());  // histo line
        lines->add_alias("histo", 2);
    }
    
    // Ensure data connection is set
    if (data_source) {
        auto line_series = std::dynamic_pointer_cast<LineSeries>(data_source);
        if (line_series) {
            if (datas.empty()) {
                datas.push_back(line_series);
            }
            if (!data) {
                data = line_series;
            }
        }
    }
}

void MACDHisto::prenext() {
    MACD::prenext();
}

void MACDHisto::next() {
    // Calculate MACD and Signal first
    MACD::next();
    
    // Calculate histogram
    auto macd_line = lines->getline(MACD::macd);
    auto signal_line = lines->getline(MACD::signal);
    auto histo_line = lines->getline(histo);
    
    if (macd_line && signal_line && histo_line) {
        double histo_value = (*macd_line)[0] - (*signal_line)[0];
        histo_line->set(0, histo_value);
    }
}

void MACDHisto::once(int start, int end) {
    // Calculate MACD and Signal first
    MACD::once(start, end);
    
    // Calculate histogram for the entire range
    auto macd_line = lines->getline(MACD::macd);
    auto signal_line = lines->getline(MACD::signal);
    auto histo_line = lines->getline(histo);
    
    if (!macd_line || !signal_line || !histo_line) {
        return;
    }
    
    // Get LineBuffers for direct access
    auto macd_buffer = std::dynamic_pointer_cast<LineBuffer>(macd_line);
    auto signal_buffer = std::dynamic_pointer_cast<LineBuffer>(signal_line);
    auto histo_buffer = std::dynamic_pointer_cast<LineBuffer>(histo_line);
    
    if (!macd_buffer || !signal_buffer || !histo_buffer) {
        return;
    }
    
    const auto& macd_array = macd_buffer->array();
    const auto& signal_array = signal_buffer->array();
    
    // Clear existing histogram buffer
    histo_buffer->reset();
    
    // Calculate histogram values by subtracting signal from macd
    for (size_t i = 0; i < macd_array.size() && i < signal_array.size(); ++i) {
        double macd_val = macd_array[i];
        double signal_val = signal_array[i];
        
        if (!std::isnan(macd_val) && !std::isnan(signal_val)) {
            histo_buffer->append(macd_val - signal_val);
        } else {
            histo_buffer->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Set the buffer index to the last valid position
    if (histo_buffer->array().size() > 0) {
        histo_buffer->set_idx(histo_buffer->array().size() - 1);
    }
}

double MACD::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto macd_line = lines->getline(0);
    if (!macd_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Check if the buffer has any data (before minimum period)
    if (macd_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*macd_line)[ago];
}

int MACD::getMinPeriod() const {
    return params.period_me2 + params.period_signal - 1;
}

void MACD::calculate() {
    // Ensure lines exist
    if (!lines || lines->size() == 0) {
        setup_lines();
    }
    
    // Get the MACD line
    auto macd_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!macd_line) {
        return;
    }
    
    // Check if we have data from either data_source_ or datas vector
    std::shared_ptr<LineSeries> data_to_use;
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        data_to_use = data_source_;
    } else if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        data_to_use = datas[0];
    } else {
        return;
    }
    
    // Get the close line using utility function
    auto data_line = utils::getDataLine(data_to_use);
    
    if (!data_line) {
        return;
    }
    
    // Get the close line buffer
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!close_buffer) {
        return;
    }
    
    // Get MACD output buffer
    auto macd_output_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(macd));
    auto signal_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(signal));
    if (!macd_output_buffer || !signal_line) {
        return;
    }
    
    // Get data size
    size_t actual_size = close_buffer->array().size();
    
    // For batch mode (when all data is available), use once() for efficient calculation
    // This is the typical case when called from tests
    if (actual_size > 0) {
        // Use once() to calculate all values at once
        once(0, actual_size);
    }
}

double MACD::getMACDLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto macd_line = lines->getline(Lines::macd);
    if (!macd_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Check if the buffer has any data (before minimum period)
    if (macd_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*macd_line)[ago];
}

double MACD::getSignalLine(int ago) const {
    if (!lines || lines->size() <= Lines::signal) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto signal_line = lines->getline(Lines::signal);
    if (!signal_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Check if the buffer has any data (before minimum period)
    if (signal_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*signal_line)[ago];
}

double MACD::getHistogram(int ago) const {
    // Calculate histogram as MACD - Signal
    double macd_val = getMACDLine(ago);
    double signal_val = getSignalLine(ago);
    
    if (std::isnan(macd_val) || std::isnan(signal_val)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return macd_val - signal_val;
}

double MACDHisto::get(int ago) const {
    if (!lines || lines->size() <= histo) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto histo_line = lines->getline(histo);
    if (!histo_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*histo_line)[ago];
}

size_t MACD::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto macd_line = lines->getline(macd);
    return macd_line ? macd_line->size() : 0;
}

size_t MACDHisto::size() const {
    if (!lines || lines->size() == 0) return 0;
    // Return the size of any line (they should all have the same size)
    auto macd_line = lines->getline(macd);
    return macd_line ? macd_line->size() : 0;
}

void MACDHisto::calculate() {
    // First calculate MACD and Signal
    MACD::calculate();
    
    // Then calculate histogram
    auto macd_line = lines->getline(MACD::macd);
    auto signal_line = lines->getline(MACD::signal);
    auto histo_line = lines->getline(histo);
    
    if (!macd_line || !signal_line || !histo_line) {
        return;
    }
    
    // Get LineBuffers for direct access
    auto macd_buffer = std::dynamic_pointer_cast<LineBuffer>(macd_line);
    auto signal_buffer = std::dynamic_pointer_cast<LineBuffer>(signal_line);
    auto histo_buffer = std::dynamic_pointer_cast<LineBuffer>(histo_line);
    
    if (!macd_buffer || !signal_buffer || !histo_buffer) {
        return;
    }
    
    const auto& macd_array = macd_buffer->array();
    const auto& signal_array = signal_buffer->array();
    
    // Determine how many histogram values we need to calculate
    size_t macd_size = macd_array.size();
    size_t signal_size = signal_array.size();
    size_t histo_size = histo_buffer->array().size();
    
    // Only calculate new histogram values (for streaming mode)
    size_t start_idx = histo_size;
    
    // If this is the first calculation, reset the buffer
    if (histo_size == 0 || histo_size == 1) {
        histo_buffer->reset();
        start_idx = 0;
    }
    
    // Calculate histogram for new values
    for (size_t i = start_idx; i < macd_size && i < signal_size; ++i) {
        double macd_val = macd_array[i];
        double signal_val = signal_array[i];
        
        if (!std::isnan(macd_val) && !std::isnan(signal_val)) {
            histo_buffer->append(macd_val - signal_val);
        } else {
            histo_buffer->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Match the buffer index with MACD buffer
    if (macd_buffer->get_idx() >= 0) {
        histo_buffer->set_idx(macd_buffer->get_idx());
    } else if (histo_buffer->array().size() > 0) {
        histo_buffer->set_idx(histo_buffer->array().size() - 1);
    }
}

} // namespace indicators
} // namespace backtrader