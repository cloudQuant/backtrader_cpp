#include "indicators/kama.h"
#include "indicators/sma.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>

namespace backtrader {

// AdaptiveMovingAverage implementation
AdaptiveMovingAverage::AdaptiveMovingAverage() 
    : Indicator(), prev_kama_(0.0), initialized_(false) {
    setup_lines();
    
    // Create SMA for initial seed value
    sma_seed_ = std::make_shared<indicators::SMA>();
    // Note: SMA params setup would be done differently based on actual SMA implementation
    
    // Calculate smoothing constants
    fast_sc_ = 2.0 / (params.fast + 1.0);   // Fast EMA smoothing factor
    slow_sc_ = 2.0 / (params.slow + 1.0);   // Slow EMA smoothing factor
    
    _minperiod(params.period + 1); // Need period + 1 for direction calculation
}

AdaptiveMovingAverage::AdaptiveMovingAverage(std::shared_ptr<LineRoot> data, int period, int fast, int slow)
    : Indicator(), prev_kama_(0.0), initialized_(false) {
    params.period = period;
    params.fast = fast;
    params.slow = slow;
    
    setup_lines();
    
    // Create SMA for initial seed value
    sma_seed_ = std::make_shared<indicators::SMA>();
    // Note: SMA params setup would be done differently based on actual SMA implementation
    
    // Calculate smoothing constants
    fast_sc_ = 2.0 / (params.fast + 1.0);   // Fast EMA smoothing factor
    slow_sc_ = 2.0 / (params.slow + 1.0);   // Slow EMA smoothing factor
    
    _minperiod(params.period + 1); // Need period + 1 for direction calculation
}

double AdaptiveMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(kama);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int AdaptiveMovingAverage::getMinPeriod() const {
    return params.period + 1;
}

void AdaptiveMovingAverage::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AdaptiveMovingAverage::prenext() {
    Indicator::prenext();
    
    // SMA will be calculated separately - avoid calling protected methods
}

void AdaptiveMovingAverage::nextstart() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to SMA seed if not already done
    if (sma_seed_->datas.empty() && !datas.empty()) {
        sma_seed_->datas = datas;
    }
    
    // Use manual SMA calculation instead of calling protected methods
    
    auto kama_line = lines->getline(kama);
    
    // Calculate simple moving average manually for seed value
    if (!datas.empty() && datas[0]->lines) {
        auto close_line = datas[0]->lines->getline(3); // Close line
        if (close_line && close_line->size() >= static_cast<size_t>(params.period)) {
            double sum = 0.0;
            for (int i = 0; i < params.period; ++i) {
                sum += (*close_line)[-i];
            }
            double sma_value = sum / params.period;
            
            if (kama_line) {
                // Initialize KAMA with SMA value
                prev_kama_ = sma_value;
                kama_line->set(0, prev_kama_);
                initialized_ = true;
            }
        }
    }
}

void AdaptiveMovingAverage::next() {
    if (!initialized_) {
        nextstart();
        return;
    }
    
    if (datas.empty() || !datas[0]->lines) return;
    
    auto kama_line = lines->getline(kama);
    auto close_line = datas[0]->lines->getline(0); // Use first line as data
    
    if (!kama_line || !close_line) return;
    
    // Calculate efficiency ratio
    double efficiency_ratio = calculate_efficiency_ratio();
    
    // Calculate smoothing constant
    double smoothing_constant = calculate_smoothing_constant(efficiency_ratio);
    
    // Calculate KAMA: KAMA = KAMA_prev + SC * (Price - KAMA_prev)
    double current_price = (*close_line)[0];
    double kama_value = prev_kama_ + smoothing_constant * (current_price - prev_kama_);
    
    kama_line->set(0, kama_value);
    prev_kama_ = kama_value;
}

void AdaptiveMovingAverage::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to SMA seed if not already done
    if (sma_seed_->datas.empty() && !datas.empty()) {
        sma_seed_->datas = datas;
    }
    
    // Calculate initial values manually
    
    auto kama_line = lines->getline(kama);
    auto close_line = datas[0]->lines->getline(3); // Close line
    
    if (!kama_line || !close_line) return;
    
    // Initialize with SMA value calculated manually
    if (start + params.period <= end) {
        double sum = 0.0;
        for (int i = 0; i < params.period; ++i) {
            sum += (*close_line)[start + i];
        }
        double sma_value = sum / params.period;
        kama_line->set(start + params.period - 1, sma_value);
        double prev_kama = sma_value;
        
        for (int i = start + params.period; i < end; ++i) {
        // Calculate direction (change over period)
        double current_price = (*close_line)[i];
        double period_ago_price = (*close_line)[i - params.period];
        double direction = std::abs(current_price - period_ago_price);
        
        // Calculate volatility (sum of absolute price changes)
        double volatility = 0.0;
        for (int j = 1; j <= params.period; ++j) {
            double price_change = std::abs((*close_line)[i - j + 1] - (*close_line)[i - j]);
            volatility += price_change;
        }
        
        // Calculate efficiency ratio
        double efficiency_ratio = (volatility != 0.0) ? direction / volatility : 0.0;
        
        // Calculate smoothing constant
        double sc_range = fast_sc_ - slow_sc_;
        double smoothing_constant = std::pow(efficiency_ratio * sc_range + slow_sc_, 2.0);
        
        // Calculate KAMA
        double kama_value = prev_kama + smoothing_constant * (current_price - prev_kama);
        
        kama_line->set(i, kama_value);
        prev_kama = kama_value;
        }
    }
}

double AdaptiveMovingAverage::calculate_efficiency_ratio() {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto close_line = datas[0]->lines->getline(0);
    if (!close_line) return 0.0;
    
    // Direction: absolute change over period
    double current_price = (*close_line)[0];
    double period_ago_price = (*close_line)[-params.period];
    double direction = std::abs(current_price - period_ago_price);
    
    // Volatility: sum of absolute daily changes
    double volatility = 0.0;
    for (int i = 1; i <= params.period; ++i) {
        double price_change = std::abs((*close_line)[-(i-1)] - (*close_line)[-i]);
        volatility += price_change;
    }
    
    // Efficiency Ratio
    return (volatility != 0.0) ? direction / volatility : 0.0;
}

double AdaptiveMovingAverage::calculate_smoothing_constant(double efficiency_ratio) {
    // SC = [ER * (fast SC - slow SC) + slow SC]^2
    double sc_range = fast_sc_ - slow_sc_;
    return std::pow(efficiency_ratio * sc_range + slow_sc_, 2.0);
}

} // namespace backtrader