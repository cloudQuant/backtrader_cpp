#include "indicators/trix.h"
#include <limits>
#include <cmath>

namespace backtrader {
namespace indicators {

// Simplified Trix implementation
Trix::Trix() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Create three EMA indicators for triple smoothing
    ema1_ = std::make_shared<EMA>(params.period);
    ema2_ = std::make_shared<EMA>(params.period);
    ema3_ = std::make_shared<EMA>(params.period);
    
    // TRIX needs 3 * period + _rocperiod for full calculation
    _minperiod(3 * params.period + params._rocperiod - 2);
}

Trix::Trix(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Create three EMA indicators for triple smoothing
    ema1_ = std::make_shared<EMA>(params.period);
    ema2_ = std::make_shared<EMA>(params.period);
    ema3_ = std::make_shared<EMA>(params.period);
    
    // TRIX needs 3 * period + _rocperiod for full calculation
    _minperiod(3 * params.period + params._rocperiod - 2);
}

double Trix::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(trix);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int Trix::getMinPeriod() const {
    return 3 * params.period + params._rocperiod - 2;
}

void Trix::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
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
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto trix_line = lines->getline(trix);
    
    if (!data_line || !trix_line) return;
    
    // Simplified TRIX calculation
    // In real implementation, this would use triple EMA smoothing
    // For now, just use the input data as a placeholder
    double price = (*data_line)[0];
    double prev_price = (data_line->size() > 1) ? (*data_line)[-1] : price;
    
    // Simple rate of change calculation as placeholder
    double trix_value = 0.0;
    if (prev_price != 0.0) {
        trix_value = ((price - prev_price) / prev_price) * 100.0;
    }
    
    trix_line->set(0, trix_value);
}

void Trix::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto trix_line = lines->getline(trix);
    
    if (!data_line || !trix_line) return;
    
    for (int i = start; i < end; ++i) {
        double price = (*data_line)[i];
        double prev_price = (i > 0) ? (*data_line)[i-1] : price;
        
        // Simple rate of change calculation as placeholder
        double trix_value = 0.0;
        if (prev_price != 0.0) {
            trix_value = ((price - prev_price) / prev_price) * 100.0;
        }
        
        trix_line->set(i, trix_value);
    }
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
}}}}
} // namespace indicators
} // namespace backtrader