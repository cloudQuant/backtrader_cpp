#include "indicators/ichimoku.h"
#include <algorithm>
#include <cmath>

namespace backtrader {

// Ichimoku implementation
Ichimoku::Ichimoku() : Indicator() {
    setup_lines();
    
    // Maximum period needed is senkou + senkou_lead for lookahead
    _minperiod(std::max({params.tenkan, params.kijun, params.senkou}) + params.senkou_lead);
    
    // Initialize storage vectors
    senkou_span_a_values_.resize(params.senkou_lead, 0.0);
    senkou_span_b_values_.resize(params.senkou_lead, 0.0);
    chikou_span_values_.resize(params.chikou, 0.0);
}

void Ichimoku::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Ichimoku::prenext() {
    Indicator::prenext();
}

void Ichimoku::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto tenkan_line = lines->getline(Lines::tenkan_sen);
    auto kijun_line = lines->getline(Lines::kijun_sen);
    auto senkou_a_line = lines->getline(Lines::senkou_span_a);
    auto senkou_b_line = lines->getline(Lines::senkou_span_b);
    auto chikou_line = lines->getline(Lines::chikou_span);
    
    if (!tenkan_line || !kijun_line || !senkou_a_line || !senkou_b_line || !chikou_line) return;
    
    // Calculate Tenkan-sen (Conversion Line)
    tenkan_line->set(0, calculate_tenkan_sen());
    
    // Calculate Kijun-sen (Base Line)
    kijun_line->set(0, calculate_kijun_sen());
    
    // Calculate Senkou Span A (Leading Span A) - shifted forward
    double senkou_a_value = calculate_senkou_span_a();
    senkou_span_a_values_.push_back(senkou_a_value);
    if (senkou_span_a_values_.size() > params.senkou_lead) {
        senkou_a_line->set(0, senkou_span_a_values_.front());
        senkou_span_a_values_.erase(senkou_span_a_values_.begin());
    }
    
    // Calculate Senkou Span B (Leading Span B) - shifted forward
    double senkou_b_value = calculate_senkou_span_b();
    senkou_span_b_values_.push_back(senkou_b_value);
    if (senkou_span_b_values_.size() > params.senkou_lead) {
        senkou_b_line->set(0, senkou_span_b_values_.front());
        senkou_span_b_values_.erase(senkou_span_b_values_.begin());
    }
    
    // Calculate Chikou Span (Lagging Span) - shifted backward
    auto close_line = datas[0]->lines->getline(3); // Close line
    if (close_line) {
        double close_value = (*close_line)[0];
        chikou_span_values_.push_back(close_value);
        if (chikou_span_values_.size() > params.chikou) {
            chikou_line->set(0, chikou_span_values_.front());
            chikou_span_values_.erase(chikou_span_values_.begin());
        }
    }
}

void Ichimoku::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto tenkan_line = lines->getline(Lines::tenkan_sen);
    auto kijun_line = lines->getline(Lines::kijun_sen);
    auto senkou_a_line = lines->getline(Lines::senkou_span_a);
    auto senkou_b_line = lines->getline(Lines::senkou_span_b);
    auto chikou_line = lines->getline(Lines::chikou_span);
    
    if (!tenkan_line || !kijun_line || !senkou_a_line || !senkou_b_line || !chikou_line) return;
    
    for (int i = start; i < end; ++i) {
        // Calculate Tenkan-sen
        tenkan_line->set(i, calculate_tenkan_sen(start - i));
        
        // Calculate Kijun-sen
        kijun_line->set(i, calculate_kijun_sen(start - i));
        
        // Calculate Senkou Span A (shifted forward)
        if (i >= params.senkou_lead) {
            senkou_a_line->set(i, calculate_senkou_span_a(start - (i - params.senkou_lead)));
        }
        
        // Calculate Senkou Span B (shifted forward)
        if (i >= params.senkou_lead) {
            senkou_b_line->set(i, calculate_senkou_span_b(start - (i - params.senkou_lead)));
        }
        
        // Calculate Chikou Span (shifted backward)
        auto close_line = datas[0]->lines->getline(3);
        if (close_line && i + params.chikou < end) {
            chikou_line->set(i + params.chikou, (*close_line)[i]);
        }
    }
}

double Ichimoku::get_highest(int period, int offset) {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    if (!high_line) return 0.0;
    
    double highest = (*high_line)[-offset];
    for (int i = 1; i < period; ++i) {
        double value = (*high_line)[-(i + offset)];
        if (value > highest) {
            highest = value;
        }
    }
    
    return highest;
}

double Ichimoku::get_lowest(int period, int offset) {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto low_line = datas[0]->lines->getline(2); // Low line
    if (!low_line) return 0.0;
    
    double lowest = (*low_line)[-offset];
    for (int i = 1; i < period; ++i) {
        double value = (*low_line)[-(i + offset)];
        if (value < lowest) {
            lowest = value;
        }
    }
    
    return lowest;
}

double Ichimoku::calculate_tenkan_sen(int offset) {
    double highest = get_highest(params.tenkan, offset);
    double lowest = get_lowest(params.tenkan, offset);
    return (highest + lowest) / 2.0;
}

double Ichimoku::calculate_kijun_sen(int offset) {
    double highest = get_highest(params.kijun, offset);
    double lowest = get_lowest(params.kijun, offset);
    return (highest + lowest) / 2.0;
}

double Ichimoku::calculate_senkou_span_a(int offset) {
    double tenkan = calculate_tenkan_sen(offset);
    double kijun = calculate_kijun_sen(offset);
    return (tenkan + kijun) / 2.0;
}

double Ichimoku::calculate_senkou_span_b(int offset) {
    double highest = get_highest(params.senkou, offset);
    double lowest = get_lowest(params.senkou, offset);
    return (highest + lowest) / 2.0;
}

} // namespace backtrader