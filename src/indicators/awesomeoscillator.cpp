#include "indicators/awesomeoscillator.h"

namespace backtrader {

// AwesomeOscillator implementation
AwesomeOscillator::AwesomeOscillator() : Indicator() {
    setup_lines();
    
    // Create SMA indicators
    sma_fast_ = std::make_shared<SMA>();
    sma_fast_->params.period = params.fast;
    
    sma_slow_ = std::make_shared<SMA>();
    sma_slow_->params.period = params.slow;
    
    _minperiod(params.slow);
}

void AwesomeOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AwesomeOscillator::prenext() {
    if (sma_fast_) sma_fast_->prenext();
    if (sma_slow_) sma_slow_->prenext();
    Indicator::prenext();
}

void AwesomeOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    auto low_line = datas[0]->lines->getline(2);  // Low line
    
    if (!high_line || !low_line) return;
    
    // Calculate median price = (high + low) / 2
    double high_value = (*high_line)[0];
    double low_value = (*low_line)[0];
    double median_price = (high_value + low_value) / 2.0;
    
    median_prices_.push_back(median_price);
    
    // Connect median prices to SMAs if not already done
    if (sma_fast_->datas.empty()) {
        auto median_if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
        auto median_line = median_lines->getline(0);
        if (median_line) {
            median_line->set(0, median_price);
        }
        sma_fast_->add_data(median_lines);
        sma_slow_->add_data(median_lines);
    }
    
    // Update SMAs
    sma_fast_->next();
    sma_slow_->next();
    
    // Calculate AO = SMA(fast) - SMA(slow)
    auto ao_line = lines->getline(Lines::ao);
    auto sma_fast_line = sma_fast_->lines->getline(SMA::Lines::sma);
    auto sma_slow_line = sma_slow_->lines->getline(SMA::Lines::sma);
    
    if (ao_line && sma_fast_line && sma_slow_line) {
        double sma_fast_value = (*sma_fast_line)[0];
        double sma_slow_value = (*sma_slow_line)[0];
        ao_line->set(0, sma_fast_value - sma_slow_value);
    }
}

void AwesomeOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    
    if (!high_line || !low_line) return;
    
    // Build median price array
    std::vector<double> all_median_prices;
    for (int i = start; i < end; ++i) {
        double high_value = (*high_line)[i];
        double low_value = (*low_line)[i];
        double median_price = (high_value + low_value) / 2.0;
        all_median_prices.push_back(median_price);
    }
    
    // Create lines for median prices
    auto median_if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
    auto median_line = median_lines->getline(0);
    
    if (median_line) {
        for (size_t i = 0; i < all_median_prices.size(); ++i) {
            median_line->set(i, all_median_prices[i]);
        }
        
        // Connect to SMAs
        sma_fast_->add_data(median_lines);
        sma_slow_->add_data(median_lines);
        
        // Calculate SMAs
        sma_fast_->once(params.fast - 1, all_median_prices.size());
        sma_slow_->once(params.slow - 1, all_median_prices.size());
        
        // Calculate AO values
        auto ao_line = lines->getline(Lines::ao);
        auto sma_fast_line = sma_fast_->lines->getline(SMA::Lines::sma);
        auto sma_slow_line = sma_slow_->lines->getline(SMA::Lines::sma);
        
        if (ao_line && sma_fast_line && sma_slow_line) {
            for (int i = start; i < end; ++i) {
                int sma_idx = i - start;
                if (sma_idx >= 0) {
                    double sma_fast_value = (*sma_fast_line)[sma_idx];
                    double sma_slow_value = (*sma_slow_line)[sma_idx];
                    ao_line->set(i, sma_fast_value - sma_slow_value);
                }
            }
        }
    }
}

} // namespace backtrader