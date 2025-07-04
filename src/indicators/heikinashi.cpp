#include "indicators/heikinashi.h"
#include <algorithm>

namespace backtrader {

// HeikinAshi implementation
HeikinAshi::HeikinAshi() : Indicator() {
    setup_lines();
    _minperiod(1);
}

void HeikinAshi::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void HeikinAshi::prenext() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    // Seed recursive value for first bar
    double open = (*data_lines->getline(0))[0];
    double close = (*data_lines->getline(3))[0];
    
    (*lines->getline(Lines::ha_open))[0] = (open + close) / 2.0;
}

void HeikinAshi::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    // Get OHLC data
    double open = (*data_lines->getline(0))[0];
    double high = (*data_lines->getline(1))[0];
    double low = (*data_lines->getline(2))[0];
    double close = (*data_lines->getline(3))[0];
    
    // Calculate Heikin Ashi values
    double ha_close = (open + high + low + close) / 4.0;
    double ha_open;
    
    // For first bar, use seed value, otherwise use recursive formula
    if (get_line_size() == 1) {
        ha_open = (open + close) / 2.0;
    } else {
        double prev_ha_open = (*lines->getline(Lines::ha_open))[-1];
        double prev_ha_close = (*lines->getline(Lines::ha_close))[-1];
        ha_open = (prev_ha_open + prev_ha_close) / 2.0;
    }
    
    double ha_high = std::max({high, ha_open, ha_close});
    double ha_low = std::min({low, ha_open, ha_close});
    
    // Set line values
    (*lines->getline(Lines::ha_open))[0] = ha_open;
    (*lines->getline(Lines::ha_high))[0] = ha_high;
    (*lines->getline(Lines::ha_low))[0] = ha_low;
    (*lines->getline(Lines::ha_close))[0] = ha_close;
}

void HeikinAshi::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    for (int i = start; i < end; ++i) {
        // Get OHLC data
        double open = (*data_lines->getline(0))[i];
        double high = (*data_lines->getline(1))[i];
        double low = (*data_lines->getline(2))[i];
        double close = (*data_lines->getline(3))[i];
        
        // Calculate Heikin Ashi values
        double ha_close = (open + high + low + close) / 4.0;
        double ha_open;
        
        // For first bar, use seed value, otherwise use recursive formula
        if (i == start) {
            ha_open = (open + close) / 2.0;
        } else {
            double prev_ha_open = (*lines->getline(Lines::ha_open))[i - 1];
            double prev_ha_close = (*lines->getline(Lines::ha_close))[i - 1];
            ha_open = (prev_ha_open + prev_ha_close) / 2.0;
        }
        
        double ha_high = std::max({high, ha_open, ha_close});
        double ha_low = std::min({low, ha_open, ha_close});
        
        // Set line values
        (*lines->getline(Lines::ha_open))[i] = ha_open;
        (*lines->getline(Lines::ha_high))[i] = ha_high;
        (*lines->getline(Lines::ha_low))[i] = ha_low;
        (*lines->getline(Lines::ha_close))[i] = ha_close;
    }
}

// HaDelta implementation
HaDelta::HaDelta() : Indicator() {
    setup_lines();
    _minperiod(params.period);
    
    // Create Heikin Ashi indicator if autoheikin is enabled
    if (params.autoheikin) {
        heikin_ashi_ = std::make_shared<HeikinAshi>();
    }
    
    // Create SMA for smoothing
    sma_ = std::make_shared<SMA>();
    sma_->params.period = params.period;
}

void HaDelta::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void HaDelta::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto hadelta_line = lines->getline(Lines::haDelta);
    auto smoothed_line = lines->getline(Lines::smoothed);
    
    if (!hadelta_line || !smoothed_line) return;
    
    double ha_open, ha_close;
    
    if (params.autoheikin && heikin_ashi_) {
        // Use Heikin Ashi transformed data
        heikin_ashi_->datas = datas;
        heikin_ashi_->next();
        
        if (heikin_ashi_->lines && 
            heikin_ashi_->lines->getline(HeikinAshi::Lines::ha_open) &&
            heikin_ashi_->lines->getline(HeikinAshi::Lines::ha_close)) {
            
            ha_open = (*heikin_ashi_->lines->getline(HeikinAshi::Lines::ha_open))[0];
            ha_close = (*heikin_ashi_->lines->getline(HeikinAshi::Lines::ha_close))[0];
        } else {
            ha_open = 0.0;
            ha_close = 0.0;
        }
    } else {
        // Use regular OHLC data
        auto data_lines = datas[0]->lines;
        ha_open = (*data_lines->getline(0))[0];  // Open
        ha_close = (*data_lines->getline(3))[0]; // Close
    }
    
    // Calculate haDelta
    double ha_delta = ha_close - ha_open;
    hadelta_line->set(0, ha_delta);
    
    // Calculate smoothed haDelta using SMA
    // For simplicity, calculate SMA manually here
    if (get_line_size() >= params.period) {
        double sum = 0.0;
        for (int i = 0; i < params.period; ++i) {
            sum += (*hadelta_line)[-i];
        }
        smoothed_line->set(0, sum / params.period);
    } else {
        smoothed_line->set(0, ha_delta);
    }
}

void HaDelta::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto hadelta_line = lines->getline(Lines::haDelta);
    auto smoothed_line = lines->getline(Lines::smoothed);
    
    if (!hadelta_line || !smoothed_line) return;
    
    // Calculate Heikin Ashi if needed
    if (params.autoheikin && heikin_ashi_) {
        heikin_ashi_->datas = datas;
        heikin_ashi_->once(start, end);
    }
    
    for (int i = start; i < end; ++i) {
        double ha_open, ha_close;
        
        if (params.autoheikin && heikin_ashi_ && heikin_ashi_->lines) {
            // Use Heikin Ashi transformed data
            if (heikin_ashi_->lines->getline(HeikinAshi::Lines::ha_open) &&
                heikin_ashi_->lines->getline(HeikinAshi::Lines::ha_close)) {
                
                ha_open = (*heikin_ashi_->lines->getline(HeikinAshi::Lines::ha_open))[i];
                ha_close = (*heikin_ashi_->lines->getline(HeikinAshi::Lines::ha_close))[i];
            } else {
                ha_open = 0.0;
                ha_close = 0.0;
            }
        } else {
            // Use regular OHLC data
            auto data_lines = datas[0]->lines;
            ha_open = (*data_lines->getline(0))[i];  // Open
            ha_close = (*data_lines->getline(3))[i]; // Close
        }
        
        // Calculate haDelta
        double ha_delta = ha_close - ha_open;
        hadelta_line->set(i, ha_delta);
        
        // Calculate smoothed haDelta using SMA
        if (i >= start + params.period - 1) {
            double sum = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum += (*hadelta_line)[i - j];
            }
            smoothed_line->set(i, sum / params.period);
        } else {
            smoothed_line->set(i, ha_delta);
        }
    }
}

} // namespace backtrader