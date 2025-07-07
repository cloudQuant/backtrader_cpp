#include "indicators/smmaosc.h"
#include <limits>

namespace backtrader {
namespace indicators {

SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // SMMA indicators will be initialized in next() when data is available
    smma_fast_ = nullptr;
    smma_slow_ = nullptr;
    
    _minperiod(params.slow);
}

SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    smma_fast_ = std::make_shared<SMMA>(data_source, params.fast);
    smma_slow_ = std::make_shared<SMMA>(data_source, params.slow);
    
    _minperiod(params.slow);
}

SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int fast, int slow) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.fast = fast;
    params.slow = slow;
    setup_lines();
    
    smma_fast_ = std::make_shared<SMMA>(data_source, params.fast);
    smma_slow_ = std::make_shared<SMMA>(data_source, params.slow);
    
    _minperiod(params.slow);
}

SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator(std::shared_ptr<LineRoot> data) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Add data source to datas for traditional indicator interface
    if (data) {
        auto data_series = std::dynamic_pointer_cast<LineSeries>(data);
        if (data_series) {
            datas.push_back(data_series);
        }
    }
    
    // SMMA indicators will be initialized in next() when data is available
    smma_fast_ = nullptr;
    smma_slow_ = nullptr;
    
    _minperiod(params.slow);
}

SmoothedMovingAverageOscillator::SmoothedMovingAverageOscillator(std::shared_ptr<LineRoot> data, int fast, int slow) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.fast = fast;
    params.slow = slow;
    setup_lines();
    
    // Add data source to datas for traditional indicator interface
    if (data) {
        auto data_series = std::dynamic_pointer_cast<LineSeries>(data);
        if (data_series) {
            datas.push_back(data_series);
        }
    }
    
    // SMMA indicators will be initialized in next() when data is available
    smma_fast_ = nullptr;
    smma_slow_ = nullptr;
    
    _minperiod(params.slow);
}

double SmoothedMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(smmaosc);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int SmoothedMovingAverageOscillator::getMinPeriod() const {
    return params.slow;
}

void SmoothedMovingAverageOscillator::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        if (smma_fast_) smma_fast_->calculate();
        if (smma_slow_) smma_slow_->calculate();
        current_index_++;
        
        // Calculate oscillator
        auto osc_line = lines->getline(smmaosc);
        auto fast_line = smma_fast_ ? smma_fast_->lines->getline(0) : nullptr;
        auto slow_line = smma_slow_ ? smma_slow_->lines->getline(0) : nullptr;
        
        if (osc_line && fast_line && slow_line) {
            double fast_value = (*fast_line)[0];
            double slow_value = (*slow_line)[0];
            
            if (!std::isnan(fast_value) && !std::isnan(slow_value)) {
                double osc_value = fast_value - slow_value;
                osc_line->set(0, osc_value);
            }
        }
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void SmoothedMovingAverageOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void SmoothedMovingAverageOscillator::prenext() {
    Indicator::prenext();
}

void SmoothedMovingAverageOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Initialize SMMA indicators if not already done
    if (!smma_fast_ && !datas.empty()) {
        smma_fast_ = std::make_shared<SMMA>(datas[0], params.fast);
    }
    if (!smma_slow_ && !datas.empty()) {
        smma_slow_ = std::make_shared<SMMA>(datas[0], params.slow);
    }
    
    // Connect data to SMMAosc if not already done
    if (smma_fast_ && smma_fast_->datas.empty() && !datas.empty()) {
        smma_fast_->datas = datas;
    }
    if (smma_slow_ && smma_slow_->datas.empty() && !datas.empty()) {
        smma_slow_->datas = datas;
    }
    
    // Update SMMA indicators using calculate method
    if (smma_fast_) {
        smma_fast_->calculate();
    }
    if (smma_slow_) {
        smma_slow_->calculate();
    }
    
    auto osc_line = lines->getline(smmaosc);
    auto fast_line = smma_fast_ ? smma_fast_->lines->getline(0) : nullptr;
    auto slow_line = smma_slow_ ? smma_slow_->lines->getline(0) : nullptr;
    
    if (osc_line && fast_line && slow_line) {
        double fast_value = (*fast_line)[0];
        double slow_value = (*slow_line)[0];
        
        if (!std::isnan(fast_value) && !std::isnan(slow_value)) {
            double osc_value = fast_value - slow_value;
            osc_line->set(0, osc_value);
        }
    }
}

void SmoothedMovingAverageOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Initialize SMMA indicators if not already done
    if (!smma_fast_ && !datas.empty()) {
        smma_fast_ = std::make_shared<SMMA>(datas[0], params.fast);
    }
    if (!smma_slow_ && !datas.empty()) {
        smma_slow_ = std::make_shared<SMMA>(datas[0], params.slow);
    }
    
    // Connect data to SMMA indicators if not already done
    if (smma_fast_ && smma_fast_->datas.empty() && !datas.empty()) {
        smma_fast_->datas = datas;
    }
    if (smma_slow_ && smma_slow_->datas.empty() && !datas.empty()) {
        smma_slow_->datas = datas;
    }
    
    // Calculate SMMA indicators for the range using calculate method
    if (smma_fast_ && smma_slow_) {
        for (int i = start; i < end; ++i) {
            smma_fast_->calculate();
            smma_slow_->calculate();
        }
    }
    
    auto osc_line = lines->getline(smmaosc);
    auto fast_line = smma_fast_ ? smma_fast_->lines->getline(0) : nullptr;
    auto slow_line = smma_slow_ ? smma_slow_->lines->getline(0) : nullptr;
    
    if (!osc_line || !fast_line || !slow_line) return;
    
    for (int i = start; i < end; ++i) {
        if (i < static_cast<int>(fast_line->size()) && i < static_cast<int>(slow_line->size())) {
            double fast_value = (*fast_line)[i];
            double slow_value = (*slow_line)[i];
            
            if (!std::isnan(fast_value) && !std::isnan(slow_value)) {
                double osc_value = fast_value - slow_value;
                osc_line->set(i, osc_value);
            }
        }
    }
}

} // namespace indicators
} // namespace backtrader