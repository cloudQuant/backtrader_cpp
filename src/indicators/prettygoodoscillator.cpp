#include "indicators/prettygoodoscillator.h"
#include <limits>

namespace backtrader {

// PrettyGoodOscillator implementation
PrettyGoodOscillator::PrettyGoodOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

PrettyGoodOscillator::PrettyGoodOscillator(std::shared_ptr<LineRoot> data_source, int period) 
    : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Add data source to datas for traditional indicator interface
    if (data_source) {
        auto data_series = std::dynamic_pointer_cast<LineSeries>(data_source);
        if (data_series) {
            datas.push_back(data_series);
        }
    }
}

double PrettyGoodOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(pgo);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int PrettyGoodOscillator::getMinPeriod() const {
    return params.period;
}

void PrettyGoodOscillator::calculate() {
    next();
}

void PrettyGoodOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void PrettyGoodOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto pgo_line = lines->getline(pgo);
    
    if (!data_line || !pgo_line) return;
    
    // Create SMA if not exists
    if (!sma_) {
        sma_ = std::make_shared<indicators::SMA>(datas[0], params.period);
    }
    
    // Create ATR if not exists  
    if (!atr_) {
        atr_ = std::make_shared<indicators::ATR>(datas[0], params.period);
    }
    
    // Calculate SMA and ATR
    sma_->calculate();
    atr_->calculate();
    
    // Pretty Good Oscillator formula: (Close - SMA) / ATR
    double current_price = (*data_line)[0];
    double sma_value = sma_->get(0);
    double atr_value = atr_->get(0);
    
    if (!std::isnan(sma_value) && !std::isnan(atr_value) && atr_value != 0.0) {
        double pgo_value = (current_price - sma_value) / atr_value;
        pgo_line->set(0, pgo_value);
    } else {
        pgo_line->set(0, 0.0);
    }
}

void PrettyGoodOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto pgo_line = lines->getline(pgo);
    
    if (!data_line || !pgo_line) return;
    
    // Simple implementation: just iterate and call next() for each position
    for (int i = start; i < end; ++i) {
        if (i >= params.period - 1) {
            // For once method, we simulate the calculation at each point
            // This is a simplified implementation
            double sum = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum += (*data_line)[i - j];
            }
            double sma_value = sum / params.period;
            
            // Simple ATR approximation for the once method
            double tr = std::abs((*data_line)[i] - (*data_line)[i-1]);
            double atr_value = tr; // Simplified ATR
            
            double current_price = (*data_line)[i];
            if (atr_value != 0.0) {
                double pgo_value = (current_price - sma_value) / atr_value;
                pgo_line->set(i, pgo_value);
            } else {
                pgo_line->set(i, 0.0);
            }
        }
    }
}

} // namespace backtrader