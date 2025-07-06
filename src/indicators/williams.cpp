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
    auto close_line = datas[0]->lines->getline(3); // Close line
    
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
    auto close_line = datas[0]->lines->getline(3); // Close line
    
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
    }
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
    }
    
    return lowest;
}

// WilliamsAD implementation
WilliamsAD::WilliamsAD() : Indicator(), accumulated_ad_(0.0) {
    setup_lines();
    
    // Create TrueHigh and TrueLow indicators
    truehigh_ = std::make_shared<TrueHigh>();
    truelow_ = std::make_shared<TrueLow>();
    
    _minperiod(2); // Need previous close for up/down day calculation
}

void WilliamsAD::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void WilliamsAD::prenext() {
    if (truehigh_) truehigh_->prenext();
    if (truelow_) truelow_->prenext();
    Indicator::prenext();
}

void WilliamsAD::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto ad_line = lines->getline(ad);
    auto close_line = datas[0]->lines->getline(3); // Close line
    
    if (!ad_line || !close_line) return;
    
    // Connect data to sub-indicators if not already done
    if (truehigh_->datas.empty() && !datas.empty()) {
        truehigh_->datas = datas;
    }
    if (truelow_->datas.empty() && !datas.empty()) {
        truelow_->datas = datas;
    }
    
    // Update sub-indicators
    truehigh_->next();
    truelow_->next();
    
    double current_close = (*close_line)[0];
    double prev_close = (*close_line)[-1];
    
    auto truehigh_line = truehigh_->lines->getline(TrueHigh::truehigh);
    auto truelow_line = truelow_->lines->getline(TrueLow::truelow);
    
    if (!truehigh_line || !truelow_line) return;
    
    double true_high = (*truehigh_line)[0];
    double true_low = (*truelow_line)[0];
    
    double ad_value = 0.0;
    
    if (is_upday(current_close, prev_close)) {
        // Up day: close - true_low
        ad_value = current_close - true_low;
    } else if (is_downday(current_close, prev_close)) {
        // Down day: close - true_high
        ad_value = current_close - true_high;
    }
    
    accumulated_ad_ += ad_value;
    ad_line->set(0, accumulated_ad_);
}

void WilliamsAD::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to sub-indicators if not already done
    if (truehigh_->datas.empty() && !datas.empty()) {
        truehigh_->datas = datas;
    }
    if (truelow_->datas.empty() && !datas.empty()) {
        truelow_->datas = datas;
    }
    
    // Calculate sub-indicators
    truehigh_->once(start, end);
    truelow_->once(start, end);
    
    auto ad_line = lines->getline(ad);
    auto close_line = datas[0]->lines->getline(3); // Close line
    
    if (!ad_line || !close_line) return;
    
    auto truehigh_line = truehigh_->lines->getline(TrueHigh::truehigh);
    auto truelow_line = truelow_->lines->getline(TrueLow::truelow);
    
    if (!truehigh_line || !truelow_line) return;
    
    double accumulated = (start > 0) ? (*ad_line)[start - 1] : 0.0;
    
    for (int i = start; i < end; ++i) {
        double current_close = (*close_line)[i];
        double prev_close = (i > 0) ? (*close_line)[i - 1] : current_close;
        
        double true_high = (*truehigh_line)[i];
        double true_low = (*truelow_line)[i];
        
        double ad_value = 0.0;
        
        if (is_upday(current_close, prev_close)) {
            ad_value = current_close - true_low;
        } else if (is_downday(current_close, prev_close)) {
            ad_value = current_close - true_high;
        }
        
        accumulated += ad_value;
        ad_line->set(i, accumulated);
    }
}

bool WilliamsAD::is_upday(double current_close, double prev_close) {
    return current_close > prev_close;
}

bool WilliamsAD::is_downday(double current_close, double prev_close) {
    return current_close < prev_close;
}

} // namespace backtrader