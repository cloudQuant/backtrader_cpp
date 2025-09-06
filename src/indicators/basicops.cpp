#include "indicators/basicops.h"
#include "dataseries.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

// PeriodN implementation
PeriodN::PeriodN() : Indicator() {
    setup_minperiod();
}

void PeriodN::setup_minperiod() {
    _minperiod(params.period);
}

// OperationN implementation
OperationN::OperationN() : PeriodN() {
    setup_lines();
}

void OperationN::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void OperationN::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Use adaptive line selection like SMA
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() > 4) {
        // For DataSeries: use line specified by subclass
        data_line = datas[0]->lines->getline(get_dataseries_line_index());
    } else {
        // For LineSeries: use line 0
        data_line = datas[0]->lines->getline(0);
    }
    
    auto result_line = lines->getline(0);
    
    if (!data_line || !result_line) return;
    
    // Get period data
    std::vector<double> period_data;
    for (int i = params.period - 1; i >= 0; --i) {
        period_data.push_back((*data_line)[-i]);
    }
    
    // Calculate result
    double result = calculate_func(period_data);
    result_line->set(0, result);
}

void OperationN::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Use adaptive line selection like SMA
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() > 4) {
        // For DataSeries: use line specified by subclass
        data_line = datas[0]->lines->getline(get_dataseries_line_index());
    } else {
        // For LineSeries: use line 0
        data_line = datas[0]->lines->getline(0);
    }
    
    auto result_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    
    if (!data_line || !result_line) return;
    
    // Clear and prepare the result buffer
    result_line->reset();
    
    // Get direct access to the data array if it's a LineBuffer
    auto linebuf = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!linebuf) return;
    
    const auto& data_array = linebuf->array();
    
    // Process all positions from start to end
    // Debug: print loop range
    // Removed debug output
    
    for (int i = start; i < end; ++i) {
        double result = std::numeric_limits<double>::quiet_NaN();
        
        // Only calculate if we have enough data
        if (i >= params.period - 1) {
            // Get period data directly from array
            std::vector<double> period_data;
            for (int j = 0; j < params.period; ++j) {
                // For position i with period p, we want values from (i - p + 1) to i
                // When j=0, we want data_index = i - p + 1
                // When j=p-1, we want data_index = i
                int data_index = i - params.period + 1 + j;
                if (data_index >= 0 && data_index < static_cast<int>(data_array.size())) {
                    period_data.push_back(data_array[data_index]);
                }
            }
            
            // Calculate result if we have enough data
            if (static_cast<int>(period_data.size()) == params.period) {
                result = calculate_func(period_data);
            }
        }
        
        
        // For the first value, replace the initial NaN
        if (i == start && start == 0) {
            result_line->set(0, result);
        } else {
            result_line->append(result);
        }
    }
    
    // Position _idx at the last value in the buffer
    // This ensures that get(0) returns the most recent calculated value
    int buffer_size = result_line->array().size();
    if (buffer_size > 0) {
        result_line->set_idx(buffer_size - 1);
    }
}

// BaseApplyN implementation
BaseApplyN::BaseApplyN() : OperationN() {
    // params.func should be set by user
}

double BaseApplyN::calculate_func(const std::vector<double>& data) {
    if (params.func) {
        return params.func(data);
    }
    return 0.0;
}

// ApplyN implementation
ApplyN::ApplyN() : BaseApplyN() {
    setup_lines();
}

void ApplyN::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

// Highest implementation
Highest::Highest() : OperationN(), data_source_(nullptr), current_index_(0) {
    params.period = 30;  // Default period to match Python backtrader
    setup_lines();
    _minperiod(params.period);
}

Highest::Highest(std::shared_ptr<LineSeries> data_source, int period) 
    : OperationN(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

Highest::Highest(std::shared_ptr<DataSeries> data_source, int period) 
    : OperationN(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

double Highest::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(highest);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int Highest::getMinPeriod() const {
    return _minperiod();
}

size_t Highest::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto highest_line = lines->getline(highest);
    if (!highest_line) {
        return 0;
    }
    return highest_line->size();
}

void Highest::calculate() {
    // Get data source
    std::shared_ptr<LineSeries> data_to_use;
    if (data_source_) {
        data_to_use = data_source_;
    } else if (data && data->lines && data->lines->size() > 0) {
        data_to_use = data;
    } else if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        data_to_use = datas[0];
    } else {
        return;
    }
    
    // Set up datas for OperationN if not already done
    if (datas.empty() && data_to_use) {
        datas.push_back(data_to_use);
    }
    
    // Get the data line
    std::shared_ptr<LineSingle> data_line;
    if (data_to_use->lines->size() > 4) {
        // For DataSeries: use line specified by subclass (high line for Highest)
        data_line = data_to_use->lines->getline(get_dataseries_line_index());
    } else {
        // For LineSeries: use line 0
        data_line = data_to_use->lines->getline(0);
    }
    
    if (!data_line) {
        return;
    }
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    auto result_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    
    if (!data_buffer || !result_buffer) {
        return;
    }
    
    // Determine mode: streaming (size==0) or forward mode (size>0)
    if (data_buffer->size() == 0) {
        // Streaming mode: calculate all values at once
        size_t actual_size = data_buffer->array().size();
        if (actual_size > 0 && result_buffer->size() == 0) {
            // Only calculate if not already done
            once(0, actual_size);
        }
    } else {
        // Forward mode: calculate single value at current position
        int current_idx = data_buffer->get_idx();
        
        // Check if we have enough data for minimum period
        if (current_idx < params.period - 1) {
            // Not enough data - set NaN at current position
            result_buffer->set(0, std::numeric_limits<double>::quiet_NaN());
            return;
        }
        
        // Call next() to calculate highest at current position
        next();
    }
}

void Highest::prenext() {
    // Before minimum period: ensure result line contains NaN
    auto result_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(highest));
    if (result_line) {
        // In streaming mode, append NaN to the result buffer
        if (result_line->size() == 0) {
            result_line->set(0, std::numeric_limits<double>::quiet_NaN());
        } else {
            result_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
}

void Highest::nextstart() {
    // At minimum period: calculate first valid value using next()
    next();
}

void Highest::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

double Highest::calculate_func(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return *std::max_element(data.begin(), data.end());
}

// Note: Lowest implementation moved to dedicated lowest.cpp file

// SumN implementation
SumN::SumN() : OperationN(), data_source_(nullptr), current_index_(0) {
    setup_lines();
}

SumN::SumN(std::shared_ptr<LineSeries> data_source, int period) 
    : OperationN(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
}

SumN::SumN(std::shared_ptr<DataSeries> data_source, int period) 
    : OperationN(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Set up data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

double SumN::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(sumn);
    if (!line || line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Backtrader Python uses a special indexing scheme:
    // - indicator[0] is the most recent value
    // - indicator[-1] is also the most recent value
    // - indicator[-2] is 1 bar ago, etc.
    if (ago < 0) {
        // Convert Python negative index to positive ago
        // Python SumN[-225] means 224 bars ago (not 225!)
        int bars_ago = -ago - 1;
        // LineBuffer uses negative indices for past values
        return (*line)[-bars_ago];
    } else {
        // Non-negative indices: 0 is current, 1 is 1 bar ago
        // LineBuffer uses negative indices for past values
        return (*line)[-ago];
    }
}

int SumN::getMinPeriod() const {
    return params.period;
}

size_t SumN::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto sumn_line = lines->getline(sumn);
    if (!sumn_line) {
        return 0;
    }
    return sumn_line->size();
}

void SumN::calculate() {
    if (data_source_) {
        // For data source constructor, calculate for entire dataset
        if (data_source_->lines && data_source_->lines->size() > 0) {
            auto data_line = data_source_->lines->getline(0);
            
            // Set up datas for OperationN
            if (datas.empty()) {
                datas.push_back(data_source_);
            }
            
            // Calculate SumN for the entire dataset using once() method
            once(0, data_line->size());
        }
    } else if (data && data->lines && data->lines->size() > 0) {
        // For test framework constructor, calculate for entire dataset
        auto data_line = data->lines->getline(data->lines->size() > 4 ? 4 : 0);
        
        // Calculate SumN for the entire dataset using once() method
        once(0, data_line->size());
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void SumN::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

double SumN::calculate_func(const std::vector<double>& data) {
    return std::accumulate(data.begin(), data.end(), 0.0);
}

// AnyN implementation
AnyN::AnyN() : OperationN(), data_source_(nullptr), current_index_(0) {
    setup_lines();
}

AnyN::AnyN(std::shared_ptr<LineSeries> data_source, int period) 
    : OperationN(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
}

double AnyN::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(anyn);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int AnyN::getMinPeriod() const {
    return params.period;
}

void AnyN::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void AnyN::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

double AnyN::calculate_func(const std::vector<double>& data) {
    for (double value : data) {
        if (value != 0.0) return 1.0;
    }
    return 0.0;
}

// AllN implementation
AllN::AllN() : OperationN(), data_source_(nullptr), current_index_(0) {
    setup_lines();
}

AllN::AllN(std::shared_ptr<LineSeries> data_source, int period) 
    : OperationN(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
}

double AllN::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(alln);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int AllN::getMinPeriod() const {
    return params.period;
}

void AllN::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void AllN::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

double AllN::calculate_func(const std::vector<double>& data) {
    for (double value : data) {
        if (value == 0.0) return 0.0;
    }
    return 1.0;
}

// FindFirstIndex implementation
FindFirstIndex::FindFirstIndex() : OperationN() {
    setup_lines();
}

void FindFirstIndex::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

double FindFirstIndex::calculate_func(const std::vector<double>& data) {
    if (data.empty() || !params.evalfunc) return 0.0;
    
    double target = params.evalfunc(data);
    
    // Find first occurrence (looking backwards)
    for (int i = data.size() - 1; i >= 0; --i) {
        if (data[i] == target) {
            return data.size() - 1 - i;  // Return backwards index
        }
    }
    return std::numeric_limits<double>::quiet_NaN();  // Not found
}

// FindFirstIndexHighest implementation
FindFirstIndexHighest::FindFirstIndexHighest() : FindFirstIndex() {
    params.evalfunc = [](const std::vector<double>& data) {
        return *std::max_element(data.begin(), data.end());
    };
}

// FindFirstIndexLowest implementation
FindFirstIndexLowest::FindFirstIndexLowest() : FindFirstIndex() {
    params.evalfunc = [](const std::vector<double>& data) {
        return *std::min_element(data.begin(), data.end());
    };
}

// FindLastIndex implementation
FindLastIndex::FindLastIndex() : OperationN() {
    setup_lines();
}

void FindLastIndex::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

double FindLastIndex::calculate_func(const std::vector<double>& data) {
    if (data.empty() || !params.evalfunc) return 0.0;
    
    double target = params.evalfunc(data);
    
    // Find first occurrence (forward)
    for (int i = 0; i < data.size(); ++i) {
        if (data[i] == target) {
            return params.period - i - 1;  // Return backwards index
        }
    }
    return std::numeric_limits<double>::quiet_NaN();  // Not found
}

// FindLastIndexHighest implementation
FindLastIndexHighest::FindLastIndexHighest() : FindLastIndex() {
    params.evalfunc = [](const std::vector<double>& data) {
        return *std::max_element(data.begin(), data.end());
    };
}

// FindLastIndexLowest implementation
FindLastIndexLowest::FindLastIndexLowest() : FindLastIndex() {
    params.evalfunc = [](const std::vector<double>& data) {
        return *std::min_element(data.begin(), data.end());
    };
}

// Accum implementation
Accum::Accum() : Indicator() {
    setup_lines();
    _minperiod(1);
}

void Accum::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Accum::nextstart() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto accum_line = lines->getline(accum);
    
    if (!data_line || !accum_line) return;
    
    accum_line->set(0, params.seed + (*data_line)[0]);
}

void Accum::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto accum_line = lines->getline(accum);
    
    if (!data_line || !accum_line) return;
    
    accum_line->set(0, (*accum_line)[-1] + (*data_line)[0]);
}

void Accum::oncestart(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto accum_line = lines->getline(accum);
    
    if (!data_line || !accum_line) return;
    
    double prev = params.seed;
    for (int i = start; i < end; ++i) {
        accum_line->set(i, prev = prev + (*data_line)[i]);
    }
}

void Accum::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto accum_line = lines->getline(accum);
    
    if (!data_line || !accum_line) return;
    
    double prev = (*accum_line)[start - 1];
    for (int i = start; i < end; ++i) {
        accum_line->set(i, prev = prev + (*data_line)[i]);
    }
}

// Average implementation
Average::Average() : PeriodN(), data_source_(nullptr), current_index_(0) {
    setup_lines();
}

Average::Average(std::shared_ptr<LineSeries> data_source, int period) 
    : PeriodN(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
}

double Average::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(av);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int Average::getMinPeriod() const {
    return params.period;
}

void Average::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void Average::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Average::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto av_line = lines->getline(av);
    
    if (!data_line || !av_line) return;
    
    // Calculate sum over period
    double sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        sum += (*data_line)[-i];
    }
    
    av_line->set(0, sum / params.period);
}

void Average::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto av_line = lines->getline(av);
    
    if (!data_line || !av_line) return;
    
    for (int i = start; i < end; ++i) {
        double sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            sum += (*data_line)[i - j];
        }
    }
}

// ExponentialSmoothing implementation
ExponentialSmoothing::ExponentialSmoothing() : Average() {
    if (params.alpha == 0.0) {
        alpha_ = 2.0 / (1.0 + params.period);
    } else {
        alpha_ = params.alpha;
    }
}

void ExponentialSmoothing::nextstart() {
    // Use base class to calculate SMA as seed
    Average::next();
}

void ExponentialSmoothing::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto av_line = lines->getline(av);
    
    if (!data_line || !av_line) return;
    
    av_line->set(0, (*av_line)[-1] * alpha1_ + (*data_line)[0] * alpha_);
}

void ExponentialSmoothing::oncestart(int start, int end) {
    // Use base class to calculate SMA as seed
    Average::once(start, end);
}

void ExponentialSmoothing::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto av_line = lines->getline(av);
    
    if (!data_line || !av_line) return;
    
    double prev = (*av_line)[start - 1];
    for (int i = start; i < end; ++i) {
        av_line->set(i, prev = prev * alpha1_ + (*data_line)[i] * alpha_);
    }
}

// WeightedAverage implementation
WeightedAverage::WeightedAverage() : PeriodN() {
    setup_lines();
    
    // If no weights provided, create linear weights
    if (params.weights.empty()) {
        params.weights.resize(params.period);
        for (int i = 0; i < params.period; ++i) {
            params.weights[i] = i + 1;  // 1, 2, 3, ... period
        }
    }
}

void WeightedAverage::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void WeightedAverage::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto av_line = lines->getline(av);
    
    if (!data_line || !av_line) return;
    
    double weighted_sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        weighted_sum += (*data_line)[-(params.period - 1 - i)] * params.weights[i];
    }
    
    av_line->set(0, params.coef * weighted_sum);
}

void WeightedAverage::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto av_line = lines->getline(av);
    
    if (!data_line || !av_line) return;
    
    for (int i = start; i < end; ++i) {
        double weighted_sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            weighted_sum += (*data_line)[i - (params.period - 1 - j)] * params.weights[j];
        }
    }
}

} // namespace indicators
} // namespace backtrader