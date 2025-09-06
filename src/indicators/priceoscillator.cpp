#include "indicators/priceoscillator.h"
#include <limits>
#include <cmath>
#include <iostream>
#include "dataseries.h"

namespace backtrader {

// PriceOscBase implementation
PriceOscBase::PriceOscBase() : Indicator() {
    _minperiod(std::max(params.period1, params.period2));
}

void PriceOscBase::prenext() {
    Indicator::prenext();
}

void PriceOscBase::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Calculate oscillator
    calculate_oscillator();
}

void PriceOscBase::once(int start, int end) {
    // Base implementation - derived classes should override
}

// PriceOscillator implementation
PriceOscillator::PriceOscillator() : PriceOscBase() {
    setup_lines();
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (params.period1 + 1.0);
    alpha2_ = 2.0 / (params.period2 + 1.0);
}

PriceOscillator::PriceOscillator(std::shared_ptr<LineSeries> data_source) : PriceOscBase() {
    setup_lines();
    this->data = data_source;
    this->datas.push_back(data_source);
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (params.period1 + 1.0);
    alpha2_ = 2.0 / (params.period2 + 1.0);
}

PriceOscillator::PriceOscillator(std::shared_ptr<DataSeries> data_source) : PriceOscBase() {
    setup_lines();
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (params.period1 + 1.0);
    alpha2_ = 2.0 / (params.period2 + 1.0);
}

PriceOscillator::PriceOscillator(std::shared_ptr<LineSeries> data_source, int period1, int period2) : PriceOscBase() {
    params.period1 = period1;
    params.period2 = period2;
    setup_lines();
    _minperiod(std::max(period1, period2));
    this->data = data_source;
    this->datas.push_back(data_source);
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (params.period1 + 1.0);
    alpha2_ = 2.0 / (params.period2 + 1.0);
}

PriceOscillator::PriceOscillator(std::shared_ptr<DataSeries> data_source, int period1, int period2) : PriceOscBase() {
    params.period1 = period1;
    params.period2 = period2;
    setup_lines();
    _minperiod(std::max(period1, period2));
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (params.period1 + 1.0);
    alpha2_ = 2.0 / (params.period2 + 1.0);
}

void PriceOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void PriceOscillator::calculate_oscillator() {
    // Not used - calculation is done in calculate() method
}

double PriceOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto line = lines->getline(po);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*line)[ago];
}

int PriceOscillator::getMinPeriod() const {
    return std::max(params.period1, params.period2);
}

void PriceOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get correct line index - for DataSeries, use close (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = 4;  // Close price for DataSeries
    }
    
    auto data_line = datas[0]->lines->getline(line_index);
    if (!data_line) return;
    
    auto po_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(po));
    if (!po_line) return;
    
    // Clear line (don't use reset() which adds an initial NaN)
    po_line->clear();
    
    // Get data as array for sequential processing
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    // Get data directly from buffer in chronological order
    const auto& raw_array = data_buffer->array();
    size_t buffer_size = raw_array.size();
    if (buffer_size == 0) return;
    
    std::vector<double> data_array;
    data_array.reserve(buffer_size);
    
    // Copy data in chronological order (oldest to newest) for EMA calculation
    // Skip the initial NaN value that LineBuffer starts with
    int start_idx = 0;
    if (buffer_size > 0 && std::isnan(raw_array[0])) {
        start_idx = 1;
    }
    
    for (size_t i = start_idx; i < buffer_size; ++i) {
        data_array.push_back(raw_array[i]);
    }
    
    if (data_array.size() < static_cast<size_t>(std::max(params.period1, params.period2))) {
        return;  // Not enough data
    }
    
    // Initialize EMAs using SMA seed (like Python backtrader)
    double ema1_seed = 0.0, ema2_seed = 0.0;
    
    // Calculate SMA seeds
    for (int i = 0; i < params.period1; ++i) {
        ema1_seed += data_array[i];
    }
    ema1_seed /= params.period1;
    
    for (int i = 0; i < params.period2; ++i) {
        ema2_seed += data_array[i];
    }
    ema2_seed /= params.period2;
    
    // Initialize EMA arrays
    std::vector<double> ema1_values(data_array.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> ema2_values(data_array.size(), std::numeric_limits<double>::quiet_NaN());
    
    // Set initial seed values
    ema1_values[params.period1 - 1] = ema1_seed;
    ema2_values[params.period2 - 1] = ema2_seed;
    
    // Calculate EMA1 from period1 onwards
    for (size_t i = params.period1; i < data_array.size(); ++i) {
        ema1_values[i] = alpha1_ * data_array[i] + (1.0 - alpha1_) * ema1_values[i-1];
    }
    
    // Calculate EMA2 from period2 onwards
    for (size_t i = params.period2; i < data_array.size(); ++i) {
        ema2_values[i] = alpha2_ * data_array[i] + (1.0 - alpha2_) * ema2_values[i-1];
    }
    
    // Calculate PriceOscillator values
    int max_period = std::max(params.period1, params.period2);
    
    // Add all PO values to the buffer
    for (size_t i = 0; i < data_array.size(); ++i) {
        if (i < static_cast<size_t>(max_period - 1)) {
            // Before both EMAs are available
            po_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Calculate PO as difference between EMAs
            double po_value = ema1_values[i] - ema2_values[i];
            po_line->append(po_value);
        }
    }
    
    // Set the buffer index to the last appended element
    if (po_line->size() > 0) {
        int last_idx = po_line->size() - 1;
        po_line->set_idx(last_idx, true);
    }
}

size_t PriceOscillator::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    
    auto po_line = lines->getline(po);
    if (!po_line) {
        return 0;
    }
    
    // Return the actual size of the po line buffer
    auto po_buffer = std::dynamic_pointer_cast<LineBuffer>(po_line);
    if (!po_buffer) return 0;
    
    // Return the size minus 1 (to exclude the initial NaN that LineBuffer adds)
    size_t buffer_size = po_buffer->size();
    if (buffer_size > 0) {
        return buffer_size - 1;
    }
    return 0;
}

void PriceOscillator::calculate() {
    // Call once() to calculate all values at once
    if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        // Get correct line index - for DataSeries, use close (index 4)
        int line_index = 0;
        auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
        if (dataseries) {
            line_index = 4;  // Close price for DataSeries
        }
        
        auto data_line = datas[0]->lines->getline(line_index);
        if (data_line) {
            // Get actual data size from the array
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (data_buffer) {
                int data_size = data_buffer->array().size();
                if (data_size > 0) {
                    once(0, data_size);
                }
            }
        }
    }
}

// PercentagePriceOscillator implementation
PercentagePriceOscillator::PercentagePriceOscillator(bool use_long_denominator) 
    : PriceOscBase(), use_long_denominator_(use_long_denominator) {
    setup_lines();
    
    _minperiod(std::max(params.period1, params.period2) + params.period_signal - 1);
    
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    signal_ema_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (params.period1 + 1.0);
    alpha2_ = 2.0 / (params.period2 + 1.0);
    alpha_signal_ = 2.0 / (params.period_signal + 1.0);
}

PercentagePriceOscillator::PercentagePriceOscillator(std::shared_ptr<DataSeries> data_source, int period1, int period2, int period_signal)
    : PriceOscBase(), use_long_denominator_(true) {
    params.period1 = period1;
    params.period2 = period2;
    params.period_signal = period_signal;
    
    setup_lines();
    
    _minperiod(std::max(period1, period2) + period_signal - 1);
    
    // Add data source to datas for traditional indicator interface
    if (data_source) {
        auto line_series = std::static_pointer_cast<LineSeries>(data_source);
        if (line_series) {
            datas.push_back(line_series);
        }
    }
    
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    signal_ema_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (params.period1 + 1.0);
    alpha2_ = 2.0 / (params.period2 + 1.0);
    alpha_signal_ = 2.0 / (params.period_signal + 1.0);
}

PercentagePriceOscillator::PercentagePriceOscillator(std::shared_ptr<LineSeries> data_source)
    : PriceOscBase(), use_long_denominator_(true) {
    setup_lines();
    _minperiod(std::max(params.period1, params.period2) + params.period_signal - 1);
    
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    signal_ema_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (params.period1 + 1.0);
    alpha2_ = 2.0 / (params.period2 + 1.0);
    alpha_signal_ = 2.0 / (params.period_signal + 1.0);
}

PercentagePriceOscillator::PercentagePriceOscillator(std::shared_ptr<LineSeries> data_source, int period1, int period2, int period_signal)
    : PriceOscBase(), use_long_denominator_(true) {
    params.period1 = period1;
    params.period2 = period2;
    params.period_signal = period_signal;
    
    setup_lines();
    _minperiod(std::max(period1, period2) + period_signal - 1);
    
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Initialize EMA calculation variables
    ema1_value_ = 0.0;
    ema2_value_ = 0.0;
    signal_ema_value_ = 0.0;
    first_calculation_ = true;
    alpha1_ = 2.0 / (params.period1 + 1.0);
    alpha2_ = 2.0 / (params.period2 + 1.0);
    alpha_signal_ = 2.0 / (params.period_signal + 1.0);
}

double PercentagePriceOscillator::get(int ago) const {
    return getPPOLine(ago);
}

int PercentagePriceOscillator::getMinPeriod() const {
    return std::max(params.period1, params.period2) + params.period_signal - 1;
}

void PercentagePriceOscillator::calculate() {
    // Call once() to calculate all values at once
    if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        // Get correct line index - for DataSeries, use close (index 4)
        int line_index = 0;
        auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
        if (dataseries) {
            line_index = 4;  // Close price for DataSeries
        }
        
        auto data_line = datas[0]->lines->getline(line_index);
        if (data_line) {
            // Get actual data size from the array
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (data_buffer) {
                int data_size = data_buffer->array().size();
                if (data_size > 0) {
                    once(0, data_size);
                }
            }
        }
    }
}

double PercentagePriceOscillator::getPPOLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto line = lines->getline(ppo);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*line)[ago];
}

double PercentagePriceOscillator::getSignalLine(int ago) const {
    if (!lines || lines->size() <= 1) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto line = lines->getline(signal);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*line)[ago];
}

double PercentagePriceOscillator::getHistogramLine(int ago) const {
    if (!lines || lines->size() <= 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto line = lines->getline(histo);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*line)[ago];
}

double PercentagePriceOscillator::getHistogram(int ago) const {
    return getHistogramLine(ago);
}

size_t PercentagePriceOscillator::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    
    auto ppo_line = lines->getline(ppo);
    if (!ppo_line) {
        return 0;
    }
    
    return ppo_line->size();
}

void PercentagePriceOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // PPO
        lines->add_line(std::make_shared<LineBuffer>());  // Signal
        lines->add_line(std::make_shared<LineBuffer>());  // Histogram
    }
}

void PercentagePriceOscillator::prenext() {
    PriceOscBase::prenext();
}

void PercentagePriceOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get correct line index - for DataSeries, use close (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = 4;  // Close price for DataSeries
    }
    
    auto data_line = datas[0]->lines->getline(line_index);
    if (!data_line) return;
    
    double current_price = (*data_line)[0];
    
    // Calculate EMAs
    if (first_calculation_) {
        ema1_value_ = current_price;
        ema2_value_ = current_price;
        first_calculation_ = false;
    } else {
        ema1_value_ = alpha1_ * current_price + (1.0 - alpha1_) * ema1_value_;
        ema2_value_ = alpha2_ * current_price + (1.0 - alpha2_) * ema2_value_;
    }
    
    // Calculate PPO
    double denominator = use_long_denominator_ ? ema2_value_ : ema1_value_;
    double ppo_value = 0.0;
    if (denominator != 0.0) {
        ppo_value = 100.0 * (ema1_value_ - ema2_value_) / denominator;
    }
    
    // Update PPO line
    auto ppo_line = lines->getline(ppo);
    if (ppo_line) {
        ppo_line->set(0, ppo_value);
    }
    
    // Calculate signal EMA
    if (data_line->size() >= static_cast<size_t>(std::max(params.period1, params.period2))) {
        if (data_line->size() == static_cast<size_t>(std::max(params.period1, params.period2))) {
            signal_ema_value_ = ppo_value;
        } else {
            signal_ema_value_ = alpha_signal_ * ppo_value + (1.0 - alpha_signal_) * signal_ema_value_;
        }
        
        // Update signal line
        auto signal_line = lines->getline(signal);
        if (signal_line) {
            signal_line->set(0, signal_ema_value_);
        }
        
        // Calculate histogram
        auto histo_line = lines->getline(histo);
        if (histo_line) {
            histo_line->set(0, ppo_value - signal_ema_value_);
        }
    }
}

void PercentagePriceOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get correct line index - for DataSeries, use close (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = 4;  // Close price for DataSeries
    }
    
    auto data_line = datas[0]->lines->getline(line_index);
    if (!data_line) return;
    
    auto ppo_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ppo));
    auto signal_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(signal));
    auto histo_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(histo));
    
    if (!ppo_line || !signal_line || !histo_line) return;
    
    // Clear lines (don't use reset() which adds an initial NaN)
    ppo_line->clear();
    signal_line->clear();
    histo_line->clear();
    
    
    // Get data as array for sequential processing
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    // Get data directly from buffer in chronological order
    const auto& raw_array = data_buffer->array();
    size_t buffer_size = raw_array.size();
    if (buffer_size == 0) return;
    
    std::vector<double> data_array;
    data_array.reserve(buffer_size);
    
    // Copy data in chronological order (oldest to newest) for EMA calculation
    // Skip the initial NaN value that LineBuffer starts with
    int start_idx = 0;
    if (buffer_size > 0 && std::isnan(raw_array[0])) {
        start_idx = 1;
    }
    
    for (size_t i = start_idx; i < buffer_size; ++i) {
        data_array.push_back(raw_array[i]);
    }
    
    if (data_array.size() < static_cast<size_t>(std::max(params.period1, params.period2))) {
        return;  // Not enough data
    }
    
    // Initialize EMAs using SMA seed (like Python backtrader)
    double ema1_seed = 0.0, ema2_seed = 0.0;
    
    // Calculate SMA seeds
    for (int i = 0; i < params.period1; ++i) {
        ema1_seed += data_array[i];
    }
    ema1_seed /= params.period1;
    
    for (int i = 0; i < params.period2; ++i) {
        ema2_seed += data_array[i];
    }
    ema2_seed /= params.period2;
    
    // Initialize EMA values with seeds
    double ema1 = ema1_seed;
    double ema2 = ema2_seed;
    
    // First pass: calculate PPO values
    std::vector<double> ppo_values;
    ppo_values.reserve(data_array.size());
    
    // Add NaN values for the initial period where EMAs haven't started
    int max_period = std::max(params.period1, params.period2);
    for (int i = 0; i < max_period - 1; ++i) {
        ppo_values.push_back(std::numeric_limits<double>::quiet_NaN());
        ppo_line->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Start EMA calculations from max_period - 1
    for (size_t i = max_period - 1; i < data_array.size(); ++i) {
        double current_price = data_array[i];
        
        // Calculate EMAs (start after the seed period)
        if (i >= static_cast<size_t>(params.period1 - 1)) {
            if (i == static_cast<size_t>(params.period1 - 1)) {
                ema1 = ema1_seed;  // Use seed for first calculation
            } else {
                ema1 = alpha1_ * current_price + (1.0 - alpha1_) * ema1;
            }
        }
        
        if (i >= static_cast<size_t>(params.period2 - 1)) {
            if (i == static_cast<size_t>(params.period2 - 1)) {
                ema2 = ema2_seed;  // Use seed for first calculation
            } else {
                ema2 = alpha2_ * current_price + (1.0 - alpha2_) * ema2;
            }
        }
        
        // Calculate PPO only when both EMAs are available
        double ppo_value = std::numeric_limits<double>::quiet_NaN();
        if (i >= static_cast<size_t>(max_period - 1)) {
            double denominator = use_long_denominator_ ? ema2 : ema1;
            if (denominator != 0.0) {
                ppo_value = 100.0 * (ema1 - ema2) / denominator;
            }
        }
        
        ppo_values.push_back(ppo_value);
        ppo_line->append(ppo_value);
    }
    
    // Second pass: calculate signal line and histogram
    double signal_ema = 0.0;
    int signal_start = std::max(params.period1, params.period2) - 1;
    
    // Calculate SMA seed for signal line (like Python backtrader)
    double signal_seed = 0.0;
    int seed_count = 0;
    for (size_t i = signal_start; i < ppo_values.size() && seed_count < params.period_signal; ++i) {
        if (!std::isnan(ppo_values[i])) {
            signal_seed += ppo_values[i];
            seed_count++;
        }
    }
    if (seed_count > 0) {
        signal_seed /= seed_count;
    }
    signal_ema = signal_seed;
    
    // Track how many valid PPO values we've seen for signal calculation
    int valid_ppo_count = 0;
    
    for (size_t i = 0; i < ppo_values.size(); ++i) {
        double ppo_value = ppo_values[i];
        
        if (i >= static_cast<size_t>(signal_start) && !std::isnan(ppo_value)) {
            // Calculate signal line (EMA of PPO with SMA seed)
            if (valid_ppo_count < params.period_signal - 1) {
                // Still building the seed period, use seed value
                signal_line->append(std::numeric_limits<double>::quiet_NaN());
                histo_line->append(std::numeric_limits<double>::quiet_NaN());
                valid_ppo_count++;
            } else if (valid_ppo_count == params.period_signal - 1) {
                // First signal output after seed period
                signal_ema = signal_seed;
                signal_line->append(signal_ema);
                histo_line->append(ppo_value - signal_ema);
                valid_ppo_count++;
            } else {
                // Normal EMA calculation
                signal_ema = alpha_signal_ * ppo_value + (1.0 - alpha_signal_) * signal_ema;
                signal_line->append(signal_ema);
                histo_line->append(ppo_value - signal_ema);
            }
        } else {
            // Before signal period, fill with NaN
            signal_line->append(std::numeric_limits<double>::quiet_NaN());
            histo_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
}

void PercentagePriceOscillator::calculate_oscillator() {
    // This is called by the base class next(), but we override next() 
    // so this method is not needed for PPO
}

// PercentagePriceOscillatorShort implementation
PercentagePriceOscillatorShort::PercentagePriceOscillatorShort() 
    : PercentagePriceOscillator(false) {  // Use short MA as denominator
}

PercentagePriceOscillatorShort::PercentagePriceOscillatorShort(std::shared_ptr<LineSeries> data_source)
    : PercentagePriceOscillator(false) {  // Use short MA as denominator
    this->data = data_source;
    this->datas.push_back(data_source);
}

PercentagePriceOscillatorShort::PercentagePriceOscillatorShort(std::shared_ptr<DataSeries> data_source)
    : PercentagePriceOscillator(data_source, 12, 26, 9) {  // Use parent constructor which sets use_long_denominator to true
    use_long_denominator_ = false;  // Explicitly set to use short MA as denominator
    // Alpha values are already initialized in parent constructor
}

PercentagePriceOscillatorShort::PercentagePriceOscillatorShort(std::shared_ptr<LineSeries> data_source, int period1, int period2, int period_signal)
    : PercentagePriceOscillator(false) {  // Use short MA as denominator
    params.period1 = period1;
    params.period2 = period2; 
    params.period_signal = period_signal;
    setup_lines();
    _minperiod(std::max(period1, period2) + period_signal - 1);
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

PercentagePriceOscillatorShort::PercentagePriceOscillatorShort(std::shared_ptr<DataSeries> data_source, int period1, int period2, int period_signal)
    : PercentagePriceOscillator(data_source, period1, period2, period_signal) {
    use_long_denominator_ = false;  // Explicitly set to use short MA as denominator
}

} // namespace backtrader