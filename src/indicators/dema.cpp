#include "indicators/dema.h"
#include "indicators/ema.h"
#include "../include/indicator_utils.h"
#include <numeric>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

// DoubleExponentialMovingAverage implementation
DoubleExponentialMovingAverage::DoubleExponentialMovingAverage() : Indicator(), data_source_(nullptr), current_index_(0), ema1_(nullptr), ema2_(nullptr) {
    setup_lines();
    
    // DEMA needs 2 * period - 1 for full calculation
    _minperiod(2 * params.period - 1);
}

DoubleExponentialMovingAverage::DoubleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0), ema1_(nullptr), ema2_(nullptr) {
    setup_lines();
    
    // DEMA needs 2 * period - 1 for full calculation
    _minperiod(2 * params.period - 1);
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Initialize sub-indicators
    initialize_sub_indicators();
}

DoubleExponentialMovingAverage::DoubleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0), ema1_(nullptr), ema2_(nullptr) {
    params.period = period;
    setup_lines();
    
    // DEMA needs 2 * period - 1 for full calculation
    _minperiod(2 * params.period - 1);
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Initialize sub-indicators
    initialize_sub_indicators();
}

DoubleExponentialMovingAverage::DoubleExponentialMovingAverage(std::shared_ptr<DataSeries> data_source)
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0), ema1_(nullptr), ema2_(nullptr) {
    setup_lines();
    
    // DEMA needs 2 * period - 1 for full calculation
    _minperiod(2 * params.period - 1);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Initialize sub-indicators
    initialize_sub_indicators();
}

DoubleExponentialMovingAverage::DoubleExponentialMovingAverage(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0), ema1_(nullptr), ema2_(nullptr) {
    params.period = period;
    setup_lines();
    
    // DEMA needs 2 * period - 1 for full calculation
    _minperiod(2 * params.period - 1);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Initialize sub-indicators
    initialize_sub_indicators();
}

double DoubleExponentialMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) return std::numeric_limits<double>::quiet_NaN();
    auto dema_line = lines->getline(dema);
    if (!dema_line) return std::numeric_limits<double>::quiet_NaN();
    
    // Handle negative indices like Python
    if (ago < 0) {
        // Python-style negative indexing
        // -1 is the last element, -2 is second to last, etc.
        auto buffer = std::dynamic_pointer_cast<LineBuffer>(dema_line);
        if (buffer) {
            const auto& arr = buffer->array();
            int idx = arr.size() + ago;
            if (idx >= 0 && idx < static_cast<int>(arr.size())) {
                return arr[idx];
            }
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use DEMA buffer's operator[] which handles indexing properly
    return (*dema_line)[ago];
}

int DoubleExponentialMovingAverage::getMinPeriod() const {
    return 2 * params.period - 1;
}

void DoubleExponentialMovingAverage::calculate() {
    // Get data size to determine proper end parameter
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        // Use getDataSize utility which handles LineBuffer _idx=-1 case
        data_size = utils::getDataSize(datas[0]);
    } else if (data_source_) {
        data_size = utils::getDataSize(data_source_);
    } else if (data) {
        data_size = utils::getDataSize(data);
    }
    
    
    // Call once() with proper range
    if (data_size > 0) {
        once(0, static_cast<int>(data_size));
    }
}

void DoubleExponentialMovingAverage::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    
    // Connect LineCollection to IndicatorBase lines_ vector for size() method
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
}

void DoubleExponentialMovingAverage::prenext() {
    Indicator::prenext();
}

void DoubleExponentialMovingAverage::next() {
    calculate();
}

void DoubleExponentialMovingAverage::initialize_sub_indicators() {
    if (!data_source_ && !datas.empty() && datas[0]) {
        data_source_ = datas[0];
    }
    
    if (!data_source_) return;
    
    // Create first EMA on the data source
    ema1_ = std::make_shared<EMA>(data_source_, params.period);
    
    // Create second EMA on the first EMA's output
    // We need to create a LineSeries that wraps the EMA1 output
    auto ema1_line_series = std::make_shared<LineSeries>();
    ema1_line_series->lines = ema1_->lines;  // Share the lines with EMA1
    
    ema2_ = std::make_shared<EMA>(ema1_line_series, params.period);
}

void DoubleExponentialMovingAverage::once(int start, int end) {
    // Get data source
    std::shared_ptr<LineSeries> source_data;
    if (!datas.empty() && datas[0]) {
        source_data = datas[0];
    } else if (data_source_) {
        source_data = data_source_;
    } else if (data) {
        source_data = std::dynamic_pointer_cast<LineSeries>(data);
    }
    
    if (!source_data) return;
    
    auto dema_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(dema));
    if (!dema_line) return;
    
    auto data_line = utils::getDataLine(source_data);
    if (!data_line) return;
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    // Get the actual data size using the array
    const auto& data_array = data_buffer->array();
    size_t data_size = data_array.size();
    
    if (data_size == 0) return;
    
    // Reset and prepare DEMA buffer
    dema_line->reset();
    dema_line->reserve(data_size);
    
    double alpha = 2.0 / (params.period + 1.0);
    
    // First pass: Calculate EMA1
    std::vector<double> ema1_values(data_size, std::numeric_limits<double>::quiet_NaN());
    double ema1 = 0.0;
    int valid_count = 0;
    double sum = 0.0;
    
    for (size_t i = 0; i < data_size; ++i) {
        if (std::isnan(data_array[i])) {
            continue;
        }
        
        valid_count++;
        
        if (valid_count < params.period) {
            sum += data_array[i];
        } else if (valid_count == params.period) {
            // Initialize with SMA
            sum += data_array[i];
            ema1 = sum / params.period;
            ema1_values[i] = ema1;
        } else {
            // Update EMA
            ema1 = alpha * data_array[i] + (1.0 - alpha) * ema1;
            ema1_values[i] = ema1;
        }
    }
    
    // Second pass: Calculate EMA2 (EMA of EMA1)
    std::vector<double> ema2_values(data_size, std::numeric_limits<double>::quiet_NaN());
    double ema2 = 0.0;
    valid_count = 0;
    sum = 0.0;
    
    for (size_t i = 0; i < data_size; ++i) {
        if (std::isnan(ema1_values[i])) {
            continue;
        }
        
        valid_count++;
        
        if (valid_count < params.period) {
            sum += ema1_values[i];
        } else if (valid_count == params.period) {
            // Initialize with SMA
            sum += ema1_values[i];
            ema2 = sum / params.period;
            ema2_values[i] = ema2;
        } else {
            // Update EMA
            ema2 = alpha * ema1_values[i] + (1.0 - alpha) * ema2;
            ema2_values[i] = ema2;
        }
    }
    
    // Third pass: Calculate DEMA = 2 * EMA1 - EMA2
    int valid_dema_count = 0;
    for (size_t i = 0; i < data_size; ++i) {
        if (!std::isnan(ema1_values[i]) && !std::isnan(ema2_values[i])) {
            double dema_val = 2.0 * ema1_values[i] - ema2_values[i];
            dema_line->append(dema_val);
            valid_dema_count++;
        } else {
            dema_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    
    // Finalize the line buffer
    utils::finalizeLineBuffer(dema_line);
}

// TripleExponentialMovingAverage implementation
TripleExponentialMovingAverage::TripleExponentialMovingAverage() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // TEMA needs cascaded EMA minperiod calculation: EMA1(30) -> EMA2(59) -> EMA3(88)
    // This matches Python's addminperiod(-1) logic for overlapping data points
    int ema1_minperiod = params.period;                              // 30
    int ema2_minperiod = ema1_minperiod + params.period - 1;         // 30 + 30 - 1 = 59
    int ema3_minperiod = ema2_minperiod + params.period - 1;         // 59 + 30 - 1 = 88
    _minperiod(ema3_minperiod);
}

TripleExponentialMovingAverage::TripleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source) : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    // TEMA needs cascaded EMA minperiod calculation: EMA1(30) -> EMA2(59) -> EMA3(88)
    // This matches Python's addminperiod(-1) logic for overlapping data points
    int ema1_minperiod = params.period;                              // 30
    int ema2_minperiod = ema1_minperiod + params.period - 1;         // 30 + 30 - 1 = 59
    int ema3_minperiod = ema2_minperiod + params.period - 1;         // 59 + 30 - 1 = 88
    _minperiod(ema3_minperiod);
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

TripleExponentialMovingAverage::TripleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period) : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // TEMA needs cascaded EMA minperiod calculation: EMA1(30) -> EMA2(59) -> EMA3(88)
    // This matches Python's addminperiod(-1) logic for overlapping data points
    int ema1_minperiod = params.period;                              // 30
    int ema2_minperiod = ema1_minperiod + params.period - 1;         // 30 + 30 - 1 = 59
    int ema3_minperiod = ema2_minperiod + params.period - 1;         // 59 + 30 - 1 = 88
    _minperiod(ema3_minperiod);
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

TripleExponentialMovingAverage::TripleExponentialMovingAverage(std::shared_ptr<DataSeries> data_source) : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    
    // TEMA needs cascaded EMA minperiod calculation: EMA1(30) -> EMA2(59) -> EMA3(88)
    // This matches Python's addminperiod(-1) logic for overlapping data points
    int ema1_minperiod = params.period;                              // 30
    int ema2_minperiod = ema1_minperiod + params.period - 1;         // 30 + 30 - 1 = 59
    int ema3_minperiod = ema2_minperiod + params.period - 1;         // 59 + 30 - 1 = 88
    _minperiod(ema3_minperiod);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

TripleExponentialMovingAverage::TripleExponentialMovingAverage(std::shared_ptr<DataSeries> data_source, int period) : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // TEMA needs cascaded EMA minperiod calculation: EMA1(30) -> EMA2(59) -> EMA3(88)
    // This matches Python's addminperiod(-1) logic for overlapping data points
    int ema1_minperiod = params.period;                              // 30
    int ema2_minperiod = ema1_minperiod + params.period - 1;         // 30 + 30 - 1 = 59
    int ema3_minperiod = ema2_minperiod + params.period - 1;         // 59 + 30 - 1 = 88
    _minperiod(ema3_minperiod);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

double TripleExponentialMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto tema_line = lines->getline(tema);
    if (!tema_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Handle negative indices like Python
    if (ago <= 0) {
        // Python-style negative indexing
        // ago=0 is the last element, ago=-1 is second to last, etc.
        auto buffer = std::dynamic_pointer_cast<LineBuffer>(tema_line);
        if (buffer) {
            const auto& arr = buffer->array();
            int idx = static_cast<int>(arr.size()) - 1 + ago;  // Fixed: ago=0 -> last element
            if (idx >= 0 && idx < static_cast<int>(arr.size())) {
                return arr[idx];
            }
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*tema_line)[ago];
}

int TripleExponentialMovingAverage::getMinPeriod() const {
    return 3 * params.period - 2;
}

void TripleExponentialMovingAverage::calculate() {
    // Get data size from parent LineSeries/DataSeries
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        data_size = utils::getDataSize(datas[0]);
    } else if (data_source_) {
        data_size = utils::getDataSize(data_source_);
    } else if (this->data) {
        data_size = utils::getDataSize(this->data);
    }
    
    if (data_size > 0) {
        // Use once() method for batch processing all data points
        once(0, data_size);
    }
}

void TripleExponentialMovingAverage::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void TripleExponentialMovingAverage::prenext() {
    Indicator::prenext();
}

void TripleExponentialMovingAverage::next() {
    // Note: This method is maintained for compatibility but we use once() for calculations
    if (datas.empty() || !datas[0]) return;
    
    // Calculate TEMA using direct math (not sub-indicators)
    auto data_line = utils::getDataLine(datas[0]);
    auto tema_line = lines->getline(tema);
    
    if (!data_line || !tema_line) return;
    
    double value = (*data_line)[0];
    if (std::isnan(value)) {
        tema_line->set(0, std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // For the next() method, we'll maintain simple state for single-step calculation
    // This is mainly for compatibility - batch calculation is handled by once()
    tema_line->set(0, std::numeric_limits<double>::quiet_NaN());
}

void TripleExponentialMovingAverage::once(int start, int end) {
    // Get data source
    std::shared_ptr<LineSeries> input_data;
    if (!datas.empty() && datas[0]) {
        input_data = std::dynamic_pointer_cast<LineSeries>(datas[0]);
    } else if (data_source_) {
        input_data = data_source_;
    } else if (data) {
        input_data = std::dynamic_pointer_cast<LineSeries>(data);
    }
    
    if (!input_data) return;
    
    auto tema_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(tema));
    if (!tema_line) return;
    
    // Clear the line buffer and reset to initial state
    tema_line->reset();
    
    int period = params.period;
    
    // Debug: Log parameters
    if (period == 30) {
        int ema1_minperiod = period;                              // 30
        int ema2_minperiod = ema1_minperiod + period - 1;         // 30 + 30 - 1 = 59
        int ema3_minperiod = ema2_minperiod + period - 1;         // 59 + 30 - 1 = 88
        std::cout << "TEMA::once() - period=" << period 
                  << ", cascaded_min_period=" << ema3_minperiod
                  << " (EMA1:" << ema1_minperiod << "->EMA2:" << ema2_minperiod << "->EMA3:" << ema3_minperiod << ")"
                  << ", data_points=" << (end - start)
                  << ", start=" << start 
                  << ", end=" << end << std::endl;
    }
    
    // Create three EMA instances following Python implementation
    // tema = 3 * ema1 - 3 * ema2 + ema3
    auto ema1 = std::make_shared<EMA>(input_data, period);
    
    // Calculate EMA1 first
    ema1->calculate();
    
    // Create a LineSeries wrapper for EMA1 output to feed into EMA2
    auto ema1_lineseries = std::make_shared<LineSeries>();
    ema1_lineseries->lines->add_line(std::dynamic_pointer_cast<LineSingle>(ema1->lines->getline(0)));
    ema1_lineseries->lines->add_alias("ema1", 0);
    
    auto ema2 = std::make_shared<EMA>(ema1_lineseries, period);
    ema2->calculate();
    
    // Create a LineSeries wrapper for EMA2 output to feed into EMA3
    auto ema2_lineseries = std::make_shared<LineSeries>();
    ema2_lineseries->lines->add_line(std::dynamic_pointer_cast<LineSingle>(ema2->lines->getline(0)));
    ema2_lineseries->lines->add_alias("ema2", 0);
    
    auto ema3 = std::make_shared<EMA>(ema2_lineseries, period);
    ema3->calculate();
    
    // Get the input data size to ensure proper buffer sizing
    auto data_line = utils::getDataLine(input_data);
    if (!data_line) return;
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    const auto& data_array = data_buffer->array();
    size_t input_data_size = data_array.size();
    
    // Count valid (non-NaN) data points to match Python behavior exactly
    size_t valid_data_count = 0;
    for (size_t i = 0; i < input_data_size; ++i) {
        if (!std::isnan(data_array[i])) {
            valid_data_count++;
        }
    }
    
    // Get EMA buffers directly for array access
    auto ema1_buffer = std::dynamic_pointer_cast<LineBuffer>(ema1->lines->getline(0));
    auto ema2_buffer = std::dynamic_pointer_cast<LineBuffer>(ema2->lines->getline(0));
    auto ema3_buffer = std::dynamic_pointer_cast<LineBuffer>(ema3->lines->getline(0));
    
    if (!ema1_buffer || !ema2_buffer || !ema3_buffer) {
        std::cout << "ERROR: Could not get EMA buffers" << std::endl;
        return;
    }
    
    const auto& ema1_array = ema1_buffer->array();
    const auto& ema2_array = ema2_buffer->array();
    const auto& ema3_array = ema3_buffer->array();
    
    std::cout << "TEMA: EMA array sizes: " << ema1_array.size() 
              << ", " << ema2_array.size() 
              << ", " << ema3_array.size() << std::endl;
    
    // Calculate TEMA: 3 * ema1 - 3 * ema2 + ema3
    // Process all data points to match input size
    for (size_t i = 0; i < input_data_size; ++i) {
        double ema1_val = (i < ema1_array.size()) ? ema1_array[i] : std::numeric_limits<double>::quiet_NaN();
        double ema2_val = (i < ema2_array.size()) ? ema2_array[i] : std::numeric_limits<double>::quiet_NaN();
        double ema3_val = (i < ema3_array.size()) ? ema3_array[i] : std::numeric_limits<double>::quiet_NaN();
        
        // Debug output at critical indices
        if ((i == 87 || i == 171 || i == 254) && params.period == 30) {
            std::cout << "TEMA at index " << i << ": EMA1=" << ema1_val 
                      << ", EMA2=" << ema2_val << ", EMA3=" << ema3_val;
        }
        
        double tema_value;
        if (std::isnan(ema1_val) || std::isnan(ema2_val) || std::isnan(ema3_val)) {
            // Any NaN means TEMA is NaN
            tema_value = std::numeric_limits<double>::quiet_NaN();
        } else {
            // Normal TEMA calculation
            tema_value = 3.0 * ema1_val - 3.0 * ema2_val + ema3_val;
        }
        
        if ((i == 87 || i == 171 || i == 254) && params.period == 30) {
            std::cout << ", TEMA=" << tema_value << std::endl;
        }
        
        tema_line->append(tema_value);
    }
    
    // Finalize the line buffer
    if (tema_line->size() > 0) {
        tema_line->set_idx(tema_line->size() - 1);
    }
    
    // Debug: Log final buffer size
    if (params.period == 30) {
        std::cout << "TEMA::once() - Final buffer size: " << tema_line->size() 
                  << " (input data size: " << input_data_size 
                  << ", valid data count: " << valid_data_count << ")"
                  << ", NaN count at start: ";
        int nan_count = 0;
        const auto& arr = tema_line->array();
        for (size_t i = 0; i < arr.size() && i < 20; ++i) {
            if (std::isnan(arr[i])) nan_count++;
            else break;
        }
        std::cout << nan_count << std::endl;
        
        // Log values at specific check points and debug indexing
        std::cout << "TEMA debug: buffer size=" << arr.size() << std::endl;
        for (int ago_val : {0, -167, -83}) {
            int idx = static_cast<int>(arr.size()) - 1 + ago_val;  // Fixed indexing
            if (idx >= 0 && idx < static_cast<int>(arr.size())) {
                std::cout << "  ago=" << ago_val << " -> idx=" << idx << " -> value=" << arr[idx] << std::endl;
            } else {
                std::cout << "  ago=" << ago_val << " -> idx=" << idx << " -> OUT OF BOUNDS" << std::endl;
            }
        }
        
        // Also test the get() method directly
        std::cout << "TEMA get() method test:" << std::endl;
        for (int ago_val : {0, -167, -83}) {
            double value = get(ago_val);
            std::cout << "  get(" << ago_val << ") = " << value << std::endl;
        }
    }
}

size_t DoubleExponentialMovingAverage::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto dema_line = lines->getline(dema);
    if (!dema_line) {
        return 0;
    }
    return dema_line->size();
}

size_t TripleExponentialMovingAverage::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto tema_line = lines->getline(tema);
    if (!tema_line) {
        return 0;
    }
    return tema_line->size();
}

} // namespace indicators
} // namespace backtrader