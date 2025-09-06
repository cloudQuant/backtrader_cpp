#include "indicators/tsi.h"
#include "indicator_utils.h"
#include "linebuffer.h"
#include <cmath>
#include <limits>
#include <iostream>

namespace backtrader {

// TrueStrengthIndicator implementation
TrueStrengthIndicator::TrueStrengthIndicator() : Indicator() {
    setup_lines();
    
    // TSI needs pchange + period1 + period2 - 1 for full calculation
    _minperiod(params.pchange + params.period1 + params.period2 - 1);
}

TrueStrengthIndicator::TrueStrengthIndicator(std::shared_ptr<LineSeries> data_source) 
    : TrueStrengthIndicator(data_source, 25, 13) {
}

TrueStrengthIndicator::TrueStrengthIndicator(std::shared_ptr<LineSeries> data_source, int period1, int period2) 
    : Indicator() {
    params.period1 = period1;
    params.period2 = period2;
    setup_lines();
    
    // TSI needs pchange + period1 + period2 - 1 for full calculation
    _minperiod(params.pchange + params.period1 + params.period2 - 1);
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

TrueStrengthIndicator::TrueStrengthIndicator(std::shared_ptr<DataSeries> data_source) 
    : TrueStrengthIndicator(data_source, 25, 13) {
}

TrueStrengthIndicator::TrueStrengthIndicator(std::shared_ptr<DataSeries> data_source, int period1, int period2) 
    : Indicator() {
    params.period1 = period1;
    params.period2 = period2;
    setup_lines();
    
    // TSI needs pchange + period1 + period2 - 1 for full calculation
    _minperiod(params.pchange + params.period1 + params.period2 - 1);
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

void TrueStrengthIndicator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("tsi", tsi);
    }
}

void TrueStrengthIndicator::prenext() {
    // Set NaN for TSI value during warmup period
    auto tsi_line = lines->getline(tsi);
    if (tsi_line) {
        tsi_line->set(0, std::numeric_limits<double>::quiet_NaN());
    }
}

void TrueStrengthIndicator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Try to get close price line (index 3 for DataSeries), fall back to line 0 if not available
    auto data_line = datas[0]->lines->getline(datas[0]->lines->size() > 3 ? 3 : 0);
    if (!data_line) return;
    
    auto tsi_line = lines->getline(tsi);
    if (!tsi_line) return;
    
    // Check if we have enough data
    if (data_line->size() <= static_cast<size_t>(params.pchange + params.period1 + params.period2 - 2)) {
        tsi_line->set(0, std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate simple price change and its EMA manually
    double current_price = (*data_line)[0];
    double previous_price = (*data_line)[-params.pchange];
    double price_change = current_price - previous_price;
    double abs_price_change = std::abs(price_change);
    
    // For simplicity, we'll use a simple calculation instead of full EMA
    // This is a placeholder that should be replaced with proper EMA calculation
    tsi_line->set(0, 100.0 * (price_change / (abs_price_change + 0.0001)));
}

void TrueStrengthIndicator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get input data line (close price line from test)
    auto input_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? 4 : 0);
    auto tsi_line = lines->getline(tsi);
    
    if (!input_line || !tsi_line) return;
    
    // Get the LineBuffer to access the actual data array
    auto input_buffer = std::dynamic_pointer_cast<LineBuffer>(input_line);
    auto tsi_buffer = std::dynamic_pointer_cast<LineBuffer>(tsi_line);
    if (!input_buffer || !tsi_buffer) return;
    
    // Get the data array directly
    const auto& data_array = input_buffer->array();
    size_t data_size = data_array.size();
    
    if (data_size == 0) return;
    
    // Calculate price changes
    std::vector<double> price_changes(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> abs_price_changes(data_size, std::numeric_limits<double>::quiet_NaN());
    
    for (size_t i = params.pchange; i < data_size; ++i) {
        if (!std::isnan(data_array[i]) && !std::isnan(data_array[i - params.pchange])) {
            price_changes[i] = data_array[i] - data_array[i - params.pchange];
            abs_price_changes[i] = std::abs(price_changes[i]);
        }
    }
    
    // Calculate first EMA of price changes (using SMA seed like Python)
    std::vector<double> ema1_pc(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> ema1_abs_pc(data_size, std::numeric_limits<double>::quiet_NaN());
    
    double alpha1 = 2.0 / (params.period1 + 1.0);
    
    // Calculate SMA seed for first EMA (like Python's EMA implementation)
    double seed_pc = 0.0;
    double seed_abs_pc = 0.0;
    int seed_count = 0;
    int seed_start = params.pchange;
    int seed_end = std::min(params.pchange + params.period1, static_cast<int>(data_size));
    
    for (int i = seed_start; i < seed_end; ++i) {
        if (!std::isnan(price_changes[i])) {
            seed_pc += price_changes[i];
            seed_abs_pc += abs_price_changes[i];
            seed_count++;
        }
    }
    
    if (seed_count == params.period1) {
        seed_pc /= params.period1;
        seed_abs_pc /= params.period1;
    } else {
        // Not enough data for proper seed
        seed_pc = std::numeric_limits<double>::quiet_NaN();
        seed_abs_pc = std::numeric_limits<double>::quiet_NaN();
    }
    
    // Calculate first EMA
    for (size_t i = params.pchange; i < data_size; ++i) {
        if (i < static_cast<size_t>(params.pchange + params.period1 - 1)) {
            // Still accumulating for seed
            ema1_pc[i] = std::numeric_limits<double>::quiet_NaN();
            ema1_abs_pc[i] = std::numeric_limits<double>::quiet_NaN();
        } else if (i == static_cast<size_t>(params.pchange + params.period1 - 1)) {
            // Use the SMA seed value
            if (!std::isnan(seed_pc)) {
                ema1_pc[i] = seed_pc;
                ema1_abs_pc[i] = seed_abs_pc;
            } else {
                ema1_pc[i] = std::numeric_limits<double>::quiet_NaN();
                ema1_abs_pc[i] = std::numeric_limits<double>::quiet_NaN();
            }
        } else {
            // Normal EMA calculation
            if (!std::isnan(price_changes[i]) && !std::isnan(ema1_pc[i-1])) {
                ema1_pc[i] = alpha1 * price_changes[i] + (1.0 - alpha1) * ema1_pc[i-1];
                ema1_abs_pc[i] = alpha1 * abs_price_changes[i] + (1.0 - alpha1) * ema1_abs_pc[i-1];
            } else {
                ema1_pc[i] = std::numeric_limits<double>::quiet_NaN();
                ema1_abs_pc[i] = std::numeric_limits<double>::quiet_NaN();
            }
        }
    }
    
    // Calculate second EMA (double smoothing)
    std::vector<double> ema2_pc(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> ema2_abs_pc(data_size, std::numeric_limits<double>::quiet_NaN());
    
    double alpha2 = 2.0 / (params.period2 + 1.0);
    
    // Calculate SMA seed for second EMA
    double seed2_pc = 0.0;
    double seed2_abs_pc = 0.0;
    int seed2_count = 0;
    int start_idx = params.pchange + params.period1 - 1;
    int seed2_end = std::min(start_idx + params.period2, static_cast<int>(data_size));
    
    for (int i = start_idx; i < seed2_end; ++i) {
        if (!std::isnan(ema1_pc[i])) {
            seed2_pc += ema1_pc[i];
            seed2_abs_pc += ema1_abs_pc[i];
            seed2_count++;
        }
    }
    
    if (seed2_count == params.period2) {
        seed2_pc /= params.period2;
        seed2_abs_pc /= params.period2;
    } else {
        // Not enough data for proper seed
        seed2_pc = std::numeric_limits<double>::quiet_NaN();
        seed2_abs_pc = std::numeric_limits<double>::quiet_NaN();
    }
    
    // Calculate second EMA
    for (size_t i = start_idx; i < data_size; ++i) {
        if (i < static_cast<size_t>(start_idx + params.period2 - 1)) {
            // Still accumulating for seed
            ema2_pc[i] = std::numeric_limits<double>::quiet_NaN();
            ema2_abs_pc[i] = std::numeric_limits<double>::quiet_NaN();
        } else if (i == static_cast<size_t>(start_idx + params.period2 - 1)) {
            // Use the SMA seed value
            if (!std::isnan(seed2_pc)) {
                ema2_pc[i] = seed2_pc;
                ema2_abs_pc[i] = seed2_abs_pc;
            } else {
                ema2_pc[i] = std::numeric_limits<double>::quiet_NaN();
                ema2_abs_pc[i] = std::numeric_limits<double>::quiet_NaN();
            }
        } else {
            // Normal EMA calculation
            if (!std::isnan(ema1_pc[i]) && !std::isnan(ema2_pc[i-1])) {
                ema2_pc[i] = alpha2 * ema1_pc[i] + (1.0 - alpha2) * ema2_pc[i-1];
                ema2_abs_pc[i] = alpha2 * ema1_abs_pc[i] + (1.0 - alpha2) * ema2_abs_pc[i-1];
            } else {
                ema2_pc[i] = std::numeric_limits<double>::quiet_NaN();
                ema2_abs_pc[i] = std::numeric_limits<double>::quiet_NaN();
            }
        }
    }
    
    // Fill TSI output buffer
    if (tsi_buffer->array().size() == 0) {
        // Initialize buffer with same size as input
        for (size_t i = 0; i < data_size; ++i) {
            if (i == 0) {
                tsi_buffer->set(0, std::numeric_limits<double>::quiet_NaN());
            } else {
                tsi_buffer->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Calculate TSI values
    int min_period = params.pchange + params.period1 + params.period2 - 1;
    for (int i = start; i < end && i < static_cast<int>(data_size); ++i) {
        if (i < min_period - 1) {
            tsi_buffer->set(i, std::numeric_limits<double>::quiet_NaN());
        } else {
            if (!std::isnan(ema2_pc[i]) && !std::isnan(ema2_abs_pc[i]) && ema2_abs_pc[i] != 0.0) {
                double tsi_value = 100.0 * (ema2_pc[i] / ema2_abs_pc[i]);
                tsi_buffer->set(i, tsi_value);
            } else {
                tsi_buffer->set(i, std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Set the buffer index to match the input buffer's index for get() method compatibility
    int last_valid_idx = static_cast<int>(data_size) - 1;
    tsi_buffer->set_idx(last_valid_idx);
}

double TrueStrengthIndicator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return utils::initNaN();
    }
    
    auto tsi_line = lines->getline(tsi);
    if (!tsi_line) {
        return utils::initNaN();
    }
    
    // Use the LineBuffer's built-in Python-style indexing
    // ago=0 means the last value, negative ago goes back in history
    return (*tsi_line)[ago];
}

int TrueStrengthIndicator::getMinPeriod() const {
    return params.pchange + params.period1 + params.period2 - 1;
}

void TrueStrengthIndicator::calculate() {
    // Use once() method for calculation
    if (!datas.empty() && datas[0] && datas[0]->lines) {
        // Get the close price line
        auto close_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? 4 : 0);
        if (close_line) {
            auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
            if (close_buffer) {
                const auto& data_array = close_buffer->array();
                if (data_array.size() > 0) {
                    // Call once to perform the calculation
                    once(0, data_array.size());
                    
                    // Ensure the TSI buffer has the correct index for size() method
                    auto tsi_line = lines->getline(tsi);
                    auto tsi_buffer = std::dynamic_pointer_cast<LineBuffer>(tsi_line);
                    if (tsi_buffer && tsi_buffer->array().size() > 0) {
                        // Set index to last valid position so size() returns correct value
                        tsi_buffer->set_idx(static_cast<int>(data_array.size()) - 1);
                    }
                }
            }
        }
    }
}

size_t TrueStrengthIndicator::size() const {
    if (!lines || lines->size() == 0) return 0;
    try {
        auto tsi_line = lines->getline(tsi);
        return tsi_line ? tsi_line->size() : 0;
    } catch (const std::exception& e) {
        std::cerr << "Error in TSI::size(): " << e.what() << std::endl;
        throw;
    }
}

} // namespace backtrader