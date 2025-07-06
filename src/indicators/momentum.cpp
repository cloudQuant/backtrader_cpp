#include "indicators/momentum.h"
#include <limits>

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

Momentum::Momentum(std::shared_ptr<LineRoot> data) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period + 1);
}

Momentum::Momentum(std::shared_ptr<LineRoot> data, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
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
        double current_value = (*data_line)[i];
        double period_ago_value = (*data_line)[i - params.period];
        double momentum_value = current_value - period_ago_value;
        momentum_line->set(i, momentum_value);
    }
}

// MomentumOscillator implementation
MomentumOscillator::MomentumOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period + 1);
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
    
    if (data_line && momosc_line) {
        double current_value = (*data_line)[0];
        double period_ago_value = (*data_line)[-params.period];
        
        if (period_ago_value != 0.0) {
            double momosc_value = 100.0 * (current_value / period_ago_value);
            momosc_line->set(0, momosc_value);
        } else {
            momosc_line->set(0, 100.0); // Default to 100 when divisor is zero
        }
    }
}

void MomentumOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto momosc_line = lines->getline(momosc);
    
    if (!data_line || !momosc_line) return;
    
    for (int i = start; i < end; ++i) {
        double current_value = (*data_line)[i];
        double period_ago_value = (*data_line)[i - params.period];
        
        if (period_ago_value != 0.0) {
            double momosc_value = 100.0 * (current_value / period_ago_value);
            momosc_line->set(i, momosc_value);
        } else {
            momosc_line->set(i, 100.0);
        }
    }
}

// RateOfChange implementation
RateOfChange::RateOfChange() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period + 1);
}

RateOfChange::RateOfChange(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
}

double RateOfChange::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(roc);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int RateOfChange::getMinPeriod() const {
    return params.period + 1;
}

void RateOfChange::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void RateOfChange::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void RateOfChange::prenext() {
    Indicator::prenext();
}

void RateOfChange::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto roc_line = lines->getline(roc);
    
    if (data_line && roc_line) {
        double current_value = (*data_line)[0];
        double period_ago_value = (*data_line)[-params.period];
        
        if (period_ago_value != 0.0) {
            double roc_value = (current_value - period_ago_value) / period_ago_value;
            roc_line->set(0, roc_value);
        } else {
            roc_line->set(0, 0.0); // Default to 0 when divisor is zero
        }
    }
}

void RateOfChange::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto roc_line = lines->getline(roc);
    
    if (!data_line || !roc_line) return;
    
    for (int i = start; i < end; ++i) {
        double current_value = (*data_line)[i];
        double period_ago_value = (*data_line)[i - params.period];
        
        if (period_ago_value != 0.0) {
            double roc_value = (current_value - period_ago_value) / period_ago_value;
            roc_line->set(i, roc_value);
        } else {
            roc_line->set(i, 0.0);
        }
    }
}

// RateOfChange100 implementation
RateOfChange100::RateOfChange100() : Indicator() {
    setup_lines();
    
    // Create underlying ROC indicator
    roc_indicator_ = std::make_shared<RateOfChange>();
    roc_indicator_->params.period = params.period;
    
    _minperiod(params.period + 1);
}

RateOfChange100::RateOfChange100(std::shared_ptr<LineRoot> data) : Indicator() {
    setup_lines();
    
    // Create underlying ROC indicator
    roc_indicator_ = std::make_shared<RateOfChange>();
    roc_indicator_->params.period = params.period;
    
    _minperiod(params.period + 1);
}

RateOfChange100::RateOfChange100(std::shared_ptr<LineRoot> data, int period) : Indicator() {
    params.period = period;
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
        double roc_value = (*roc_line)[i];
        roc100_line->set(i, 100.0 * roc_value);
    }
}

} // namespace indicators
} // namespace backtrader