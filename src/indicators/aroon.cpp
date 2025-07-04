#include "indicators/aroon.h"
#include <algorithm>
#include <cmath>

namespace backtrader {

// AroonBase implementation
AroonBase::AroonBase(bool calc_up, bool calc_down) 
    : Indicator(), calc_up_(calc_up), calc_down_(calc_down), up_value_(0.0), down_value_(0.0) {
    // Need period + 1 for lookback because current bar is included
    _minperiod(params.period + 1);
}

void AroonBase::prenext() {
    Indicator::prenext();
}

void AroonBase::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    calculate_lines();
}

void AroonBase::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    for (int i = start; i < end; ++i) {
        calculate_lines();
    }
}

int AroonBase::find_highest_index(int period) {
    if (datas.empty() || !datas[0]->lines) return 0;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    if (!high_line) return 0;
    
    double highest = (*high_line)[0];
    int highest_idx = 0;
    
    for (int i = 1; i < period; ++i) {
        double value = (*high_line)[-i];
        if (value > highest) {
            highest = value;
            highest_idx = i;
        }
    }
    
    return highest_idx;
}

int AroonBase::find_lowest_index(int period) {
    if (datas.empty() || !datas[0]->lines) return 0;
    
    auto low_line = datas[0]->lines->getline(2); // Low line
    if (!low_line) return 0;
    
    double lowest = (*low_line)[0];
    int lowest_idx = 0;
    
    for (int i = 1; i < period; ++i) {
        double value = (*low_line)[-i];
        if (value < lowest) {
            lowest = value;
            lowest_idx = i;
        }
    }
    
    return lowest_idx;
}

// AroonUp implementation
AroonUp::AroonUp() : AroonBase(true, false) {
    setup_lines();
}

void AroonUp::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AroonUp::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto aroonup_line = lines->getline(Lines::aroonup);
    if (!aroonup_line) return;
    
    // Find index of highest high in period + 1 bars
    int highest_idx = find_highest_index(params.period + 1);
    
    // Calculate AroonUp: 100 * (period - distance to highest high) / period
    up_value_ = (100.0 / params.period) * (params.period - highest_idx);
    aroonup_line->set(0, up_value_);
}

// AroonDown implementation
AroonDown::AroonDown() : AroonBase(false, true) {
    setup_lines();
}

void AroonDown::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AroonDown::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto aroondown_line = lines->getline(Lines::aroondown);
    if (!aroondown_line) return;
    
    // Find index of lowest low in period + 1 bars
    int lowest_idx = find_lowest_index(params.period + 1);
    
    // Calculate AroonDown: 100 * (period - distance to lowest low) / period
    down_value_ = (100.0 / params.period) * (params.period - lowest_idx);
    aroondown_line->set(0, down_value_);
}

// AroonUpDown implementation
AroonUpDown::AroonUpDown() : AroonBase(true, true) {
    setup_lines();
}

void AroonUpDown::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AroonUpDown::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto aroonup_line = lines->getline(Lines::aroonup);
    auto aroondown_line = lines->getline(Lines::aroondown);
    
    if (!aroonup_line || !aroondown_line) return;
    
    // Calculate both AroonUp and AroonDown
    int highest_idx = find_highest_index(params.period + 1);
    int lowest_idx = find_lowest_index(params.period + 1);
    
    up_value_ = (100.0 / params.period) * (params.period - highest_idx);
    down_value_ = (100.0 / params.period) * (params.period - lowest_idx);
    
    aroonup_line->set(0, up_value_);
    aroondown_line->set(0, down_value_);
}

// AroonOscillator implementation
AroonOscillator::AroonOscillator() : AroonBase(true, true) {
    setup_lines();
}

void AroonOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AroonOscillator::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto aroonosc_line = lines->getline(Lines::aroonosc);
    if (!aroonosc_line) return;
    
    // Calculate both AroonUp and AroonDown
    int highest_idx = find_highest_index(params.period + 1);
    int lowest_idx = find_lowest_index(params.period + 1);
    
    up_value_ = (100.0 / params.period) * (params.period - highest_idx);
    down_value_ = (100.0 / params.period) * (params.period - lowest_idx);
    
    // AroonOscillator = AroonUp - AroonDown
    aroonosc_line->set(0, up_value_ - down_value_);
}

// AroonUpDownOscillator implementation
AroonUpDownOscillator::AroonUpDownOscillator() : AroonBase(true, true) {
    setup_lines();
}

void AroonUpDownOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AroonUpDownOscillator::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto aroonup_line = lines->getline(Lines::aroonup);
    auto aroondown_line = lines->getline(Lines::aroondown);
    auto aroonosc_line = lines->getline(Lines::aroonosc);
    
    if (!aroonup_line || !aroondown_line || !aroonosc_line) return;
    
    // Calculate both AroonUp and AroonDown
    int highest_idx = find_highest_index(params.period + 1);
    int lowest_idx = find_lowest_index(params.period + 1);
    
    up_value_ = (100.0 / params.period) * (params.period - highest_idx);
    down_value_ = (100.0 / params.period) * (params.period - lowest_idx);
    
    aroonup_line->set(0, up_value_);
    aroondown_line->set(0, down_value_);
    aroonosc_line->set(0, up_value_ - down_value_);
}

} // namespace backtrader