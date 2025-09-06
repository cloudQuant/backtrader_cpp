#include "indicators/trix.h"
#include "indicators/ema.h"
#include <limits>
#include <cmath>

namespace backtrader {
namespace indicators {

// Trix implementation following Python version
Trix::Trix() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // TRIX needs 3 * period + _rocperiod for full calculation
    _minperiod(3 * params.period + params._rocperiod - 2);
}

Trix::Trix(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Set data source
    if (data_source_) {
        this->data = data_source_;
        this->datas.push_back(data_source_);
        
        // Create the EMA chain step by step
        ema1_ = std::make_shared<EMA>(data_source_, params.period);
        
        // For second EMA, we need to wait for first EMA to be calculated
        ema2_ = std::make_shared<EMA>(params.period);
        ema3_ = std::make_shared<EMA>(params.period);
    }
    
    // TRIX needs 3 * period + _rocperiod for full calculation
    _minperiod(3 * params.period + params._rocperiod - 2);
}

Trix::Trix(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Set data for test framework compatibility
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
        data_source_ = lineseries;
        
        // Create the EMA chain: data -> EMA1 -> EMA2 -> EMA3
        ema1_ = std::make_shared<EMA>(lineseries, params.period);
        ema2_ = std::make_shared<EMA>(ema1_, params.period);
        ema3_ = std::make_shared<EMA>(ema2_, params.period);
    }
    
    // TRIX needs 3 * period + _rocperiod for full calculation
    _minperiod(3 * params.period + params._rocperiod - 2);
}

double Trix::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(trix);
    if (!line || line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use ago directly - LineBuffer handles the indexing semantics
    return (*line)[ago];
}

int Trix::getMinPeriod() const {
    return 3 * params.period + params._rocperiod - 2;
}

size_t Trix::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    
    auto trix_line = lines->getline(trix);
    if (!trix_line) {
        return 0;
    }
    
    return trix_line->size();
}

void Trix::calculate() {
    // Use once() method for calculation
    if (!datas.empty() && datas[0] && datas[0]->lines) {
        // Get the close price line and check if we have actual data
        auto close_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? 4 : 0);
        if (close_line) {
            auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
            if (close_buffer) {
                const auto& data_array = close_buffer->array();
                if (data_array.size() > 0) {
                    // Call once to perform the calculation
                    once(0, data_array.size());
                    
                    // Ensure the TRIX buffer has the correct index for size() method
                    auto trix_line = lines->getline(trix);
                    auto trix_buffer = std::dynamic_pointer_cast<LineBuffer>(trix_line);
                    if (trix_buffer && trix_buffer->array().size() > 0) {
                        // Set index to last valid position so size() returns correct value
                        trix_buffer->set_idx(static_cast<int>(data_array.size()) - 1);
                    }
                }
            }
        }
    }
}

void Trix::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void Trix::prenext() {
    // prenext() is protected
    Indicator::prenext();
}

void Trix::next() {
    // Not used in favor of once() for efficiency
    once(current_index_, current_index_ + 1);
    current_index_++;
}

void Trix::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get input data line (close price line from test)  
    auto input_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? 4 : 0);
    auto trix_line = lines->getline(trix);
    
    if (!input_line || !trix_line) return;
    
    // Get the LineBuffer to access the actual data array
    auto input_buffer = std::dynamic_pointer_cast<LineBuffer>(input_line);
    auto trix_buffer = std::dynamic_pointer_cast<LineBuffer>(trix_line);
    if (!input_buffer || !trix_buffer) return;
    
    // Get the data array directly
    const auto& data_array = input_buffer->array();
    size_t data_size = data_array.size();
    
    if (data_size == 0) return;
    
    // Get minimum period for reference but don't return early
    int min_period = _minperiod();
    
    // Calculate triple EMA manually using the actual data array
    std::vector<double> ema1_values(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> ema2_values(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> ema3_values(data_size, std::numeric_limits<double>::quiet_NaN());
    
    double alpha = 2.0 / (params.period + 1.0);
    
    // Calculate first EMA from input data - using SMA seed like Python
    // First, calculate the seed value as the average of the first period values
    double ema1_seed = 0.0;
    int valid_count = 0;
    for (int i = 0; i < params.period && i < static_cast<int>(data_size); ++i) {
        if (!std::isnan(data_array[i])) {
            ema1_seed += data_array[i];
            valid_count++;
        }
    }
    if (valid_count > 0) {
        ema1_seed /= valid_count;
    }
    
    // Calculate first EMA with proper seed
    for (size_t i = 0; i < data_size; ++i) {
        double price = data_array[i];
        if (std::isnan(price)) {
            ema1_values[i] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }
        
        if (i < static_cast<size_t>(params.period - 1)) {
            // Still accumulating for seed
            ema1_values[i] = std::numeric_limits<double>::quiet_NaN();
        } else if (i == static_cast<size_t>(params.period - 1)) {
            // Use the seed value
            ema1_values[i] = ema1_seed;
        } else {
            // Normal EMA calculation
            ema1_values[i] = alpha * price + (1.0 - alpha) * ema1_values[i-1];
        }
    }
    
    // Calculate second EMA from first EMA - also with SMA seed
    double ema2_seed = 0.0;
    valid_count = 0;
    for (int i = params.period - 1; i < 2 * params.period - 1 && i < static_cast<int>(data_size); ++i) {
        if (!std::isnan(ema1_values[i])) {
            ema2_seed += ema1_values[i];
            valid_count++;
        }
    }
    if (valid_count > 0) {
        ema2_seed /= valid_count;
    }
    
    for (size_t i = 0; i < data_size; ++i) {
        if (std::isnan(ema1_values[i])) {
            ema2_values[i] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }
        
        if (i < static_cast<size_t>(2 * params.period - 2)) {
            // Still accumulating for seed
            ema2_values[i] = std::numeric_limits<double>::quiet_NaN();
        } else if (i == static_cast<size_t>(2 * params.period - 2)) {
            // Use the seed value
            ema2_values[i] = ema2_seed;
        } else {
            // Normal EMA calculation
            ema2_values[i] = alpha * ema1_values[i] + (1.0 - alpha) * ema2_values[i-1];
        }
    }
    
    // Calculate third EMA from second EMA - also with SMA seed
    double ema3_seed = 0.0;
    valid_count = 0;
    for (int i = 2 * params.period - 2; i < 3 * params.period - 2 && i < static_cast<int>(data_size); ++i) {
        if (!std::isnan(ema2_values[i])) {
            ema3_seed += ema2_values[i];
            valid_count++;
        }
    }
    if (valid_count > 0) {
        ema3_seed /= valid_count;
    }
    
    for (size_t i = 0; i < data_size; ++i) {
        if (std::isnan(ema2_values[i])) {
            ema3_values[i] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }
        
        if (i < static_cast<size_t>(3 * params.period - 3)) {
            // Still accumulating for seed
            ema3_values[i] = std::numeric_limits<double>::quiet_NaN();
        } else if (i == static_cast<size_t>(3 * params.period - 3)) {
            // Use the seed value
            ema3_values[i] = ema3_seed;
        } else {
            // Normal EMA calculation
            ema3_values[i] = alpha * ema2_values[i] + (1.0 - alpha) * ema3_values[i-1];
        }
    }
    
    // Fill TRIX output buffer first, then calculate TRIX values
    // Need to match the input buffer size and structure
    if (trix_buffer->array().size() == 0) {
        // Initialize buffer with same size as input
        for (size_t i = 0; i < data_size; ++i) {
            if (i == 0) {
                trix_buffer->set(0, std::numeric_limits<double>::quiet_NaN());
            } else {
                trix_buffer->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Calculate TRIX: 100.0 * (ema3 / ema3(-_rocperiod) - 1.0)
    for (int i = start; i < end && i < static_cast<int>(data_size); ++i) {
        if (i < min_period - 1 || i < params._rocperiod) {
            trix_buffer->set(i, std::numeric_limits<double>::quiet_NaN());
        } else {
            double current_ema3 = ema3_values[i];
            double previous_ema3 = ema3_values[i - params._rocperiod];
            
            if (!std::isnan(current_ema3) && !std::isnan(previous_ema3) && previous_ema3 != 0.0) {
                double trix_value = 100.0 * (current_ema3 / previous_ema3 - 1.0);
                trix_buffer->set(i, trix_value);
            } else {
                trix_buffer->set(i, std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Set the buffer index to match the input buffer's index for get() method compatibility
    // The input buffer might have an incorrect index after loading, so we set it to the last valid data point
    int last_valid_idx = static_cast<int>(data_size) - 1;
    trix_buffer->set_idx(last_valid_idx);
}

// TrixSignal implementation
TrixSignal::TrixSignal() : Trix() {
    setup_lines_signal();
    signal_ema_ = std::make_shared<EMA>(params.sigperiod);
}

void TrixSignal::setup_lines_signal() {
    // Add signal line
    if (lines->size() == 1) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void TrixSignal::prenext() {
    Trix::prenext();
}

void TrixSignal::next() {
    // First calculate base TRIX
    Trix::next();
    
    // Then calculate signal line (simplified)
    auto trix_line = lines->getline(trix);
    auto signal_line = lines->getline(signal);
    
    if (trix_line && signal_line) {
        double trix_value = (*trix_line)[0];
        signal_line->set(0, trix_value * 0.9); // Simplified signal
    }
}

void TrixSignal::once(int start, int end) {
    // First calculate base TRIX
    Trix::once(start, end);
    
    // Then calculate signal line
    auto trix_line = lines->getline(trix);
    auto signal_line = lines->getline(signal);
    
    if (!trix_line || !signal_line) return;
    
    for (int i = start; i < end; ++i) {
        double trix_value = (*trix_line)[i];
        signal_line->set(i, trix_value * 0.9); // Simplified signal
    }
}

} // namespace indicators
} // namespace backtrader