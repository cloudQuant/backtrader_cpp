#include "indicators/heikinashi.h"
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

// HeikinAshi implementation
HeikinAshi::HeikinAshi() : Indicator() {
    setup_lines();
    _minperiod(1);
}

HeikinAshi::HeikinAshi(std::shared_ptr<LineRoot> data, int period) : Indicator() {
    setup_lines();
    _minperiod(1);
    
    // For test framework compatibility - use the data as close line
    if (data && !datas.empty()) {
        // Store reference for calculation
        // In a real implementation, we'd need access to OHLC data
        // For now, we'll treat this as a simple transformation
    }
}

HeikinAshi::HeikinAshi(std::shared_ptr<LineRoot> open_line, std::shared_ptr<LineRoot> high_line, 
                       std::shared_ptr<LineRoot> low_line, std::shared_ptr<LineRoot> close_line) : Indicator() {
    setup_lines();
    _minperiod(1);
    
    // Store OHLC data lines for calculation
    // Implementation will use these in calculate() method
    // For now, just ensure we have proper setup
}

double HeikinAshi::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(ha_close);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int HeikinAshi::getMinPeriod() const {
    return 1;  // HeikinAshi只需要1个周期即可开始计算
}

void HeikinAshi::calculate() {
    next();
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
    
    auto ha_open_line = lines->getline(ha_open);
    if (ha_open_line) {
        ha_open_line->set(0, (open + close) / 2.0);
    }
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
    auto ha_open_line = lines->getline(ha_open);
    auto ha_close_line = lines->getline(ha_close);
    
    if (ha_open_line && ha_close_line && ha_open_line->size() == 1) {
        ha_open = (open + close) / 2.0;
    } else if (ha_open_line && ha_close_line) {
        double prev_ha_open = (*ha_open_line)[-1];
        double prev_ha_close = (*ha_close_line)[-1];
        ha_open = (prev_ha_open + prev_ha_close) / 2.0;
    } else {
        ha_open = (open + close) / 2.0;
    }
    
    double ha_high = std::max({high, ha_open, ha_close});
    double ha_low = std::min({low, ha_open, ha_close});
    
    // Set line values
    if (ha_open_line) ha_open_line->set(0, ha_open);
    auto ha_high_line = lines->getline(ha_high);
    if (ha_high_line) ha_high_line->set(0, ha_high);
    auto ha_low_line = lines->getline(ha_low);
    if (ha_low_line) ha_low_line->set(0, ha_low);
    if (ha_close_line) ha_close_line->set(0, ha_close);
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
            double prev_ha_open = (*lines->getline(ha_open))[i - 1];
            double prev_ha_close = (*lines->getline(ha_close))[i - 1];
            ha_open = (prev_ha_open + prev_ha_close) / 2.0;
        }
        
        double ha_high = std::max({high, ha_open, ha_close});
        double ha_low = std::min({low, ha_open, ha_close});
        
        // Set line values
        auto ha_open_line = lines->getline(ha_open);
        if (ha_open_line) ha_open_line->set(i, ha_open);
        auto ha_high_line = lines->getline(ha_high);
        if (ha_high_line) ha_high_line->set(i, ha_high);
        auto ha_low_line = lines->getline(ha_low);
        if (ha_low_line) ha_low_line->set(i, ha_low);
        auto ha_close_line = lines->getline(ha_close);
        if (ha_close_line) ha_close_line->set(i, ha_close);
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
    sma_ = std::make_shared<indicators::SMA>();
    // Note: SMA params setup would be done differently based on actual SMA implementation
}

void HaDelta::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void HaDelta::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto hadelta_line = lines->getline(haDelta);
    auto smoothed_line = lines->getline(smoothed);
    
    if (!hadelta_line || !smoothed_line) return;
    
    double ha_open, ha_close;
    
    if (params.autoheikin && heikin_ashi_) {
        // Use Heikin Ashi transformed data
        heikin_ashi_->datas = datas;
        heikin_ashi_->calculate();
        
        if (heikin_ashi_->lines && 
            heikin_ashi_->lines->getline(HeikinAshi::ha_open) &&
            heikin_ashi_->lines->getline(HeikinAshi::ha_close)) {
            
            ha_open = (*heikin_ashi_->lines->getline(HeikinAshi::ha_open))[0];
            ha_close = (*heikin_ashi_->lines->getline(HeikinAshi::ha_close))[0];
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
    if (hadelta_line && hadelta_line->size() >= static_cast<size_t>(params.period)) {
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
    
    auto hadelta_line = lines->getline(haDelta);
    auto smoothed_line = lines->getline(smoothed);
    
    if (!hadelta_line || !smoothed_line) return;
    
    // Calculate Heikin Ashi if needed
    if (params.autoheikin && heikin_ashi_) {
        heikin_ashi_->datas = datas;
        // Use calculate method instead of once as it's protected
        for (int i = start; i < end; ++i) {
            heikin_ashi_->calculate();
        }
    }
    
    for (int i = start; i < end; ++i) {
        double ha_open, ha_close;
        
        if (params.autoheikin && heikin_ashi_ && heikin_ashi_->lines) {
            // Use Heikin Ashi transformed data
            if (heikin_ashi_->lines->getline(HeikinAshi::ha_open) &&
                heikin_ashi_->lines->getline(HeikinAshi::ha_close)) {
                
                ha_open = (*heikin_ashi_->lines->getline(HeikinAshi::ha_open))[i];
                ha_close = (*heikin_ashi_->lines->getline(HeikinAshi::ha_close))[i];
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

} // namespace indicators
} // namespace backtrader