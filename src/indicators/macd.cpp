#include "indicators/macd.h"
#include "indicators/ema.h"
#include <limits>

namespace backtrader {
namespace indicators {

// MACD implementation
MACD::MACD() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Set minimum period (slow EMA period + signal period - 1)
    _minperiod(params.period_me2 + params.period_signal - 1);
}

MACD::MACD(std::shared_ptr<LineSeries> data_source, int fast_period, int slow_period, int signal_period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period_me1 = fast_period;
    params.period_me2 = slow_period;
    params.period_signal = signal_period;
    
    setup_lines();
    
    // Set minimum period (slow EMA period + signal period - 1)
    _minperiod(params.period_me2 + params.period_signal - 1);
}

void MACD::setup_lines() {
    // Create 2 lines: macd and signal
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // macd line
        lines->add_line(std::make_shared<LineBuffer>());  // signal line
        lines->add_alias("macd", 0);
        lines->add_alias("signal", 1);
    }
}

void MACD::prenext() {
    // Call parent prenext
    Indicator::prenext();
}

void MACD::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get current close price
    auto close_line = datas[0]->lines->getline(0); // Assume first line is close
    if (!close_line || close_line->size() == 0) return;
    
    // For simplicity, implement EMA calculation inline
    // In practice, this would use the EMA indicator class
    
    static std::vector<double> ema12_history;
    static std::vector<double> ema26_history;
    static std::vector<double> macd_history;
    static std::vector<double> signal_history;
    
    double current_price = (*close_line)[0];
    
    // Calculate EMA12 (fast)
    double ema12;
    if (ema12_history.empty()) {
        ema12 = current_price;
    } else {
        double multiplier = 2.0 / (params.period_me1 + 1);
        ema12 = (current_price * multiplier) + (ema12_history.back() * (1 - multiplier));
    }
    ema12_history.push_back(ema12);
    
    // Calculate EMA26 (slow)
    double ema26;
    if (ema26_history.empty()) {
        ema26 = current_price;
    } else {
        double multiplier = 2.0 / (params.period_me2 + 1);
        ema26 = (current_price * multiplier) + (ema26_history.back() * (1 - multiplier));
    }
    ema26_history.push_back(ema26);
    
    // Calculate MACD line
    double macd_value = ema12 - ema26;
    macd_history.push_back(macd_value);
    
    // Set MACD line value
    auto macd_line = lines->getline(macd);
    if (macd_line) {
        macd_line->set(0, macd_value);
    }
    
    // Calculate Signal line (EMA of MACD)
    double signal_value;
    if (signal_history.empty()) {
        signal_value = macd_value;
    } else {
        double multiplier = 2.0 / (params.period_signal + 1);
        signal_value = (macd_value * multiplier) + (signal_history.back() * (1 - multiplier));
    }
    signal_history.push_back(signal_value);
    
    // Set Signal line value
    auto signal_line = lines->getline(signal);
    if (signal_line) {
        signal_line->set(0, signal_value);
    }
}

void MACD::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto close_line = datas[0]->lines->getline(0);
    if (!close_line) return;
    
    auto macd_line = lines->getline(macd);
    auto signal_line = lines->getline(signal);
    if (!macd_line || !signal_line) return;
    
    std::vector<double> ema12_values;
    std::vector<double> ema26_values;
    std::vector<double> macd_values;
    
    // Calculate EMA12 and EMA26 for the entire range
    for (int i = start; i < end; ++i) {
        double price = (*close_line)[i];
        
        // EMA12
        double ema12;
        if (ema12_values.empty()) {
            ema12 = price;
        } else {
            double multiplier = 2.0 / (params.period_me1 + 1);
            ema12 = (price * multiplier) + (ema12_values.back() * (1 - multiplier));
        }
        ema12_values.push_back(ema12);
        
        // EMA26
        double ema26;
        if (ema26_values.empty()) {
            ema26 = price;
        } else {
            double multiplier = 2.0 / (params.period_me2 + 1);
            ema26 = (price * multiplier) + (ema26_values.back() * (1 - multiplier));
        }
        ema26_values.push_back(ema26);
        
        // MACD
        double macd_val = ema12 - ema26;
        macd_values.push_back(macd_val);
        macd_line->set(i, macd_val);
    }
    
    // Calculate Signal line
    for (int i = start; i < end; ++i) {
        double signal_val;
        if (i == start) {
            signal_val = macd_values[i - start];
        } else {
            double multiplier = 2.0 / (params.period_signal + 1);
            signal_val = (macd_values[i - start] * multiplier) + 
                        ((*signal_line)[i - 1] * (1 - multiplier));
        }
        signal_line->set(i, signal_val);
    }
}

// MACDHisto implementation
MACDHisto::MACDHisto() : MACD() {
    // Add histogram line (MACD already has macd and signal lines)
    if (lines->size() < 3) {
        lines->add_line(std::make_shared<LineBuffer>());  // histo line
        lines->add_alias("histo", 2);
    }
}

void MACDHisto::prenext() {
    MACD::prenext();
}

void MACDHisto::next() {
    // Calculate MACD and Signal first
    MACD::next();
    
    // Calculate histogram
    auto macd_line = lines->getline(MACD::macd);
    auto signal_line = lines->getline(MACD::signal);
    auto histo_line = lines->getline(histo);
    
    if (macd_line && signal_line && histo_line) {
        double histo_value = (*macd_line)[0] - (*signal_line)[0];
        histo_line->set(0, histo_value);
    }
}

void MACDHisto::once(int start, int end) {
    // Calculate MACD and Signal first
    MACD::once(start, end);
    
    // Calculate histogram for the entire range
    auto macd_line = lines->getline(MACD::macd);
    auto signal_line = lines->getline(MACD::signal);
    auto histo_line = lines->getline(histo);
    
    if (macd_line && signal_line && histo_line) {
        for (int i = start; i < end; ++i) {
            double histo_value = (*macd_line)[i] - (*signal_line)[i];
            histo_line->set(i, histo_value);
        }
    }
}

double MACD::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto macd_line = lines->getline(0);
    if (!macd_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*macd_line)[ago];
}

int MACD::getMinPeriod() const {
    return params.period_me2 + params.period_signal - 1;
}

void MACD::calculate() {
    if (!data_source_ || !data_source_->lines || data_source_->lines->size() == 0) {
        return;
    }
    
    auto data_line = data_source_->lines->getline(0);
    
    if (!data_line) {
        return;
    }
    
    // Calculate MACD for the entire dataset using once() method
    once(0, data_line->size());
}

} // namespace indicators
} // namespace backtrader