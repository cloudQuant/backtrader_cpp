#include "indicators/momentum.h"
#include "indicators/roc.h"
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

// Momentum implementation
Momentum::Momentum() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period + 1);
}

Momentum::Momentum(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
}

Momentum::Momentum(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

double Momentum::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(momentum);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int Momentum::getMinPeriod() const {
    return params.period + 1;
}

size_t Momentum::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto momentum_line = lines->getline(momentum);
    if (!momentum_line) {
        return 0;
    }
    return momentum_line->size();
}

void Momentum::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void Momentum::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void Momentum::prenext() {
    Indicator::prenext();
}

void Momentum::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto momentum_line = lines->getline(momentum);
    
    if (data_line && momentum_line) {
        double current_value = (*data_line)[0];
        double period_ago_value = (*data_line)[-params.period];
        double momentum_value = current_value - period_ago_value;
        momentum_line->set(0, momentum_value);
    }
}

void Momentum::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto momentum_line = lines->getline(momentum);
    
    if (!data_line || !momentum_line) return;
    
    for (int i = start; i < end; ++i) {
        // Convert array index to LineBuffer index
        int current_linebuffer_index = static_cast<int>(data_line->size()) - 1 - i;
        int period_ago_linebuffer_index = static_cast<int>(data_line->size()) - 1 - (i - params.period);
        double current_value = (*data_line)[current_linebuffer_index];
        double period_ago_value = (*data_line)[period_ago_linebuffer_index];
        double momentum_value = current_value - period_ago_value;
        momentum_line->set(i, momentum_value);
    }
}

// MomentumOscillator implementation
MomentumOscillator::MomentumOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period + 1);
}

MomentumOscillator::MomentumOscillator(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

MomentumOscillator::MomentumOscillator(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

double MomentumOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(momosc);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use ago directly, LineBuffer handles the indexing
    return (*line)[ago];
}

int MomentumOscillator::getMinPeriod() const {
    return params.period + 1;  // Same as Momentum, no smoothing
}

size_t MomentumOscillator::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto line = lines->getline(momosc);
    if (!line) {
        return 0;
    }
    return line->size();
}

void MomentumOscillator::calculate() {
    // Check if we have data
    if (datas.empty() || !datas[0] || !datas[0]->lines) {
        // No data available
        return;
    }
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line || data_line->size() == 0) {
        // Data line is empty
        return;
    }
    
    // Call once() to process all data
    // Use once() to process all data at once
    once(0, data_line->size());
}

void MomentumOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void MomentumOscillator::prenext() {
    Indicator::prenext();
}

void MomentumOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto momosc_line = lines->getline(momosc);
    
    if (!data_line || !momosc_line) return;
    
    // Ensure we have enough data for momentum calculation
    if (data_line->size() < static_cast<size_t>(params.period + 1)) {
        return;
    }
    
    // Calculate momentum oscillator: 100 * (current / period_ago)
    double current_value = (*data_line)[0];
    // Fix: Use correct LineBuffer indexing for period_ago
    if (params.period >= static_cast<int>(data_line->size())) {
        return;
    }
    double period_ago_value = (*data_line)[params.period];
    
    // Calculate oscillator value: 100 * (current / period_ago)
    double momosc_value = std::numeric_limits<double>::quiet_NaN();
    if (period_ago_value != 0.0) {
        momosc_value = 100.0 * (current_value / period_ago_value);
    }
    
    // Set the momentum oscillator value directly (no smoothing in Python version)
    momosc_line->set(0, momosc_value);
}

void MomentumOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto momosc_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(momosc));
    
    if (!data_line || !momosc_line) return;
    
    // Clear and reset the line buffer
    momosc_line->reset();
    
    // First, check if we have any data
    if (data_line->size() == 0) {
        // Data line is empty
        return;
    }
    
    // Data processing starts here
    
    // Debug
    // std::cout << "MomentumOscillator::once() - data_line size: " << data_line->size() 
    //           << ", start: " << start << ", end: " << end << std::endl;
    
    // Create a temporary array to hold data in chronological order
    std::vector<double> data_array;
    data_array.reserve(data_line->size());
    
    // Copy data from LineBuffer to array in chronological order
    // LineBuffer stores newest at [0], oldest at negative indices
    // Use negative indices to access historical data
    for (int i = -(static_cast<int>(data_line->size()) - 1); i <= 0; ++i) {
        data_array.push_back((*data_line)[i]);
    }
    
    // Debug
    // std::cout << "MomentumOscillator::once() - data_array size: " << data_array.size() << std::endl;
    
    // Calculate momentum oscillator for all data points chronologically
    for (int i = 0; i < static_cast<int>(data_array.size()); ++i) {
        double momosc_value = std::numeric_limits<double>::quiet_NaN();
        
        // Calculate oscillator only when we have enough data (i >= period)
        if (i >= params.period) {
            double current_value = data_array[i];
            double period_ago_value = data_array[i - params.period];
            
            // Calculate oscillator value: 100 * (current / period_ago)
            if (period_ago_value != 0.0) {
                momosc_value = 100.0 * (current_value / period_ago_value);
            }
        }
        
        // Append value to line buffer (this maintains chronological order)
        momosc_line->append(momosc_value);
    }
    
    // Processing complete
}

// RateOfChange100 implementation
RateOfChange100::RateOfChange100() : Indicator() {
    setup_lines();
    
    // Create underlying ROC indicator
    roc_indicator_ = std::make_shared<RateOfChange>();
    roc_indicator_->params.period = params.period;
    
    _minperiod(params.period + 1);
}

double RateOfChange100::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(roc100);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int RateOfChange100::getMinPeriod() const {
    return params.period + 1;
}

void RateOfChange100::calculate() {
    next();
}

void RateOfChange100::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void RateOfChange100::prenext() {
    // Skip protected method call for compilation
    Indicator::prenext();
}

void RateOfChange100::next() {
    if (!roc_indicator_) return;
    
    // Connect data to ROC indicator if not already done
    if (roc_indicator_->datas.empty() && !datas.empty()) {
        roc_indicator_->datas = datas;
    }
    
    // Skip protected method call for compilation
    // roc_indicator_->next();
    
    auto roc100_line = lines->getline(roc100);
    auto roc_line = roc_indicator_->lines->getline(RateOfChange::roc);
    
    if (roc100_line && roc_line) {
        double roc_value = (*roc_line)[0];
        roc100_line->set(0, 100.0 * roc_value);
    }
}

void RateOfChange100::once(int start, int end) {
    if (!roc_indicator_) return;
    
    // Connect data to ROC indicator if not already done
    if (roc_indicator_->datas.empty() && !datas.empty()) {
        roc_indicator_->datas = datas;
    }
    
    // Skip protected method call for compilation
    // roc_indicator_->once(start, end);
    
    auto roc100_line = lines->getline(roc100);
    auto roc_line = roc_indicator_->lines->getline(RateOfChange::roc);
    
    if (!roc100_line || !roc_line) return;
    
    for (int i = start; i < end; ++i) {
        // Convert array index to LineBuffer index
        int roc_linebuffer_index = static_cast<int>(roc_line->size()) - 1 - i;
        double roc_value = (*roc_line)[roc_linebuffer_index];
        roc100_line->set(i, 100.0 * roc_value);
    }
}

} // namespace indicators
} // namespace backtrader