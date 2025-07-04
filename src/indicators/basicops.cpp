#include "indicators/basicops.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>

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
    
    auto data_line = datas[0]->lines->getline(0);
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
    
    auto data_line = datas[0]->lines->getline(0);
    auto result_line = lines->getline(0);
    
    if (!data_line || !result_line) return;
    
    for (int i = start; i < end; ++i) {
        // Get period data
        std::vector<double> period_data;
        for (int j = params.period - 1; j >= 0; --j) {
            period_data.push_back((*data_line)[i - j]);
        }
        
        // Calculate result
        double result = calculate_func(period_data);
        result_line->set(i, result);
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
    setup_lines();
}

Highest::Highest(std::shared_ptr<LineSeries> data_source, int period) 
    : OperationN(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
}

double Highest::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(Lines::highest);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int Highest::getMinPeriod() const {
    return params.period;
}

void Highest::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
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

// Lowest implementation
Lowest::Lowest() : OperationN(), data_source_(nullptr), current_index_(0) {
    setup_lines();
}

Lowest::Lowest(std::shared_ptr<LineSeries> data_source, int period) 
    : OperationN(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
}

double Lowest::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(Lines::lowest);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int Lowest::getMinPeriod() const {
    return params.period;
}

void Lowest::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void Lowest::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

double Lowest::calculate_func(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return *std::min_element(data.begin(), data.end());
}

// SumN implementation
SumN::SumN() : OperationN(), data_source_(nullptr), current_index_(0) {
    setup_lines();
}

SumN::SumN(std::shared_ptr<LineSeries> data_source, int period) 
    : OperationN(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
}

double SumN::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(Lines::sumn);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int SumN::getMinPeriod() const {
    return params.period;
}

void SumN::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
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
    
    auto line = lines->getline(Lines::anyn);
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
    
    auto line = lines->getline(Lines::alln);
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
    return 0.0;
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
    return 0.0;
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
    auto accum_line = lines->getline(Lines::accum);
    
    if (!data_line || !accum_line) return;
    
    accum_line->set(0, params.seed + (*data_line)[0]);
}

void Accum::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto accum_line = lines->getline(Lines::accum);
    
    if (!data_line || !accum_line) return;
    
    accum_line->set(0, (*accum_line)[-1] + (*data_line)[0]);
}

void Accum::oncestart(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto accum_line = lines->getline(Lines::accum);
    
    if (!data_line || !accum_line) return;
    
    double prev = params.seed;
    for (int i = start; i < end; ++i) {
        accum_line->set(i, prev = prev + (*data_line)[i]);
    }
}

void Accum::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto accum_line = lines->getline(Lines::accum);
    
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
    
    auto line = lines->getline(Lines::av);
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
    auto av_line = lines->getline(Lines::av);
    
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
    auto av_line = lines->getline(Lines::av);
    
    if (!data_line || !av_line) return;
    
    for (int i = start; i < end; ++i) {
        double sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            sum += (*data_line)[i - j];
        }
        av_line->set(i, sum / params.period);
    }
}

// ExponentialSmoothing implementation
ExponentialSmoothing::ExponentialSmoothing() : Average() {
    if (params.alpha == 0.0) {
        alpha_ = 2.0 / (1.0 + params.period);
    } else {
        alpha_ = params.alpha;
    }
    alpha1_ = 1.0 - alpha_;
}

void ExponentialSmoothing::nextstart() {
    // Use base class to calculate SMA as seed
    Average::next();
}

void ExponentialSmoothing::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto av_line = lines->getline(Lines::av);
    
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
    auto av_line = lines->getline(Lines::av);
    
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
    auto av_line = lines->getline(Lines::av);
    
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
    auto av_line = lines->getline(Lines::av);
    
    if (!data_line || !av_line) return;
    
    for (int i = start; i < end; ++i) {
        double weighted_sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            weighted_sum += (*data_line)[i - (params.period - 1 - j)] * params.weights[j];
        }
        av_line->set(i, params.coef * weighted_sum);
    }
}

} // namespace indicators
} // namespace backtrader