#include "indicators/williams.h"
#include <algorithm>
#include <cmath>

namespace backtrader {

// WilliamsR implementation
WilliamsR::WilliamsR() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

void WilliamsR::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void WilliamsR::prenext() {
    Indicator::prenext();
}

void WilliamsR::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto percr_line = lines->getline(percR);
    auto close_line = datas[0]->lines->getline(4); // Close line
    
    if (!percr_line || !close_line) return;
    
    double highest = get_highest(params.period);
    double lowest = get_lowest(params.period);
    double current_close = (*close_line)[0];
    
    // Formula: %R = -100 * (highest - close) / (highest - lowest)
    if (highest != lowest) {
        double percr_value = -100.0 * (highest - current_close) / (highest - lowest);
        percr_line->set(0, percr_value);
    } else {
        percr_line->set(0, 0.0);
    }
}

void WilliamsR::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto percr_line = lines->getline(percR);
    auto close_line = datas[0]->lines->getline(4); // Close line
    
    if (!percr_line || !close_line) return;
    
    for (int i = start; i < end; ++i) {
        double highest = get_highest(params.period);
        double lowest = get_lowest(params.period);
        double current_close = (*close_line)[i];
        
        if (highest != lowest) {
            double percr_value = -100.0 * (highest - current_close) / (highest - lowest);
            percr_line->set(i, percr_value);
        } else {
            percr_line->set(i, 0.0);
        }

double WilliamsR::get_highest(int period) {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    if (!high_line) return 0.0;
    
    double highest = (*high_line)[0];
    for (int i = 1; i < period; ++i) {
        double value = (*high_line)[-i];
        if (value > highest) {
            highest = value;
        }
    
    return highest;
}

double WilliamsR::get_lowest(int period) {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto low_line = datas[0]->lines->getline(2); // Low line
    if (!low_line) return 0.0;
    
    double lowest = (*low_line)[0];
    for (int i = 1; i < period; ++i) {
        double value = (*low_line)[-i];
        if (value < lowest) {
            lowest = value;
        }
    
    return lowest;
}

// WilliamsAD implementation moved to williamsad.cpp

} // namespace backtrader