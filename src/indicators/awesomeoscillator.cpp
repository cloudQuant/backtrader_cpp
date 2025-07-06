#include "indicators/awesomeoscillator.h"
#include <cmath>
#include <limits>

namespace backtrader {

// AwesomeOscillator implementation
AwesomeOscillator::AwesomeOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.slow);
}

AwesomeOscillator::AwesomeOscillator(std::shared_ptr<LineRoot> data) : Indicator() {
    setup_lines();
    _minperiod(params.slow);
    
    // This constructor is for test framework compatibility
    // It assumes data is a data series where high/low can be extracted
}

AwesomeOscillator::AwesomeOscillator(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low) : Indicator() {
    setup_lines();
    _minperiod(params.slow);
    
    // Store the high/low data - in a full implementation this would be handled differently
    // For now, we'll assume the data will be available through the normal data feeds
    // This constructor is for test framework compatibility
}

void AwesomeOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("ao", 0);
    }
}

double AwesomeOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto ao_line = lines->getline(ao);
    if (!ao_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*ao_line)[ago];
}

int AwesomeOscillator::getMinPeriod() const {
    return params.slow;
}

void AwesomeOscillator::prenext() {
    next();
}

void AwesomeOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    auto low_line = datas[0]->lines->getline(2);  // Low line
    auto ao_line = lines->getline(ao);
    
    if (!high_line || !low_line || !ao_line) return;
    
    // Calculate median price = (high + low) / 2
    double high_value = (*high_line)[0];
    double low_value = (*low_line)[0];
    double median_price = (high_value + low_value) / 2.0;
    
    // Add to median price history
    median_prices_.push_back(median_price);
    
    // Keep only what we need for the longest SMA
    if (median_prices_.size() > params.slow * 2) {
        median_prices_.erase(median_prices_.begin());
    }
    
    // Need enough data for both SMAs
    if (median_prices_.size() < params.slow) {
        ao_line->set(0, std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate SMA fast (last 5 values)
    double sum_fast = 0.0;
    for (int i = 0; i < params.fast; ++i) {
        sum_fast += median_prices_[median_prices_.size() - 1 - i];
    }
    double sma_fast = sum_fast / params.fast;
    
    // Calculate SMA slow (last 34 values)
    double sum_slow = 0.0;
    for (int i = 0; i < params.slow; ++i) {
        sum_slow += median_prices_[median_prices_.size() - 1 - i];
    }
    double sma_slow = sum_slow / params.slow;
    
    // AO = SMA(fast) - SMA(slow)
    ao_line->set(0, sma_fast - sma_slow);
}

void AwesomeOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    auto ao_line = lines->getline(ao);
    
    if (!high_line || !low_line || !ao_line) return;
    
    // Build median price array for all data
    std::vector<double> all_median_prices;
    for (int i = 0; i < end; ++i) {
        double high_value = (*high_line)[i];
        double low_value = (*low_line)[i];
        double median_price = (high_value + low_value) / 2.0;
        all_median_prices.push_back(median_price);
    }
    
    // Calculate AO for each bar from start to end
    for (int i = start; i < end; ++i) {
        if (i < params.slow - 1) {
            ao_line->set(i, std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        // Calculate SMA fast (last 5 values)
        double sum_fast = 0.0;
        for (int j = 0; j < params.fast; ++j) {
            sum_fast += all_median_prices[i - j];
        }
        double sma_fast = sum_fast / params.fast;
        
        // Calculate SMA slow (last 34 values)
        double sum_slow = 0.0;
        for (int j = 0; j < params.slow; ++j) {
            sum_slow += all_median_prices[i - j];
        }
        double sma_slow = sum_slow / params.slow;
        
        // AO = SMA(fast) - SMA(slow)
        ao_line->set(i, sma_fast - sma_slow);
    }
}

} // namespace backtrader