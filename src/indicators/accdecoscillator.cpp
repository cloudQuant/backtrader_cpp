#include "indicators/accdecoscillator.h"
#include <limits>
#include <cmath>

namespace backtrader {

// AccelerationDecelerationOscillator implementation
AccelerationDecelerationOscillator::AccelerationDecelerationOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period + 34);  // AO needs at least 34 periods, plus SMA period
    
    // Create component indicators
    awesome_oscillator_ = std::make_shared<AwesomeOscillator>();
}

AccelerationDecelerationOscillator::AccelerationDecelerationOscillator(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low) : Indicator() {
    setup_lines();
    _minperiod(params.period + 34);  // AO needs at least 34 periods, plus SMA period
    
    // Create component indicators with high/low data
    awesome_oscillator_ = std::make_shared<AwesomeOscillator>();
    
    // Store high/low data for later use
    // This is a simplified approach - in a full implementation,
    // we'd need to properly set up the data feeds
}

void AccelerationDecelerationOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("accde", 0);
    }
}

double AccelerationDecelerationOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto accde_line = lines->getline(accde);
    if (!accde_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*accde_line)[ago];
}

int AccelerationDecelerationOscillator::getMinPeriod() const {
    return params.period + 34; // AO needs 34 periods, plus SMA period
}

void AccelerationDecelerationOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    auto low_line = datas[0]->lines->getline(2);  // Low line
    auto accde_line = lines->getline(accde);
    
    if (!high_line || !low_line || !accde_line) return;
    
    // Calculate median price = (high + low) / 2
    double high_value = (*high_line)[0];
    double low_value = (*low_line)[0];
    double median_price = (high_value + low_value) / 2.0;
    
    // Store median prices for AO calculation
    median_prices_.push_back(median_price);
    
    // Keep only what we need
    if (median_prices_.size() > 100) {
        median_prices_.erase(median_prices_.begin());
    }
    
    // Need enough data for AO calculation (34 periods for slow SMA)
    if (median_prices_.size() < 34) {
        accde_line->set(0, std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate AO = SMA(5) - SMA(34) of median prices
    double sum_fast = 0.0, sum_slow = 0.0;
    
    for (int i = 0; i < 5; ++i) {
        sum_fast += median_prices_[median_prices_.size() - 1 - i];
    }
    double sma_fast = sum_fast / 5.0;
    
    for (int i = 0; i < 34; ++i) {
        sum_slow += median_prices_[median_prices_.size() - 1 - i];
    }
    double sma_slow = sum_slow / 34.0;
    
    double ao_value = sma_fast - sma_slow;
    
    // Store AO values for AccDec calculation
    ao_values_.push_back(ao_value);
    
    // Keep only what we need for SMA
    if (ao_values_.size() > params.period * 2) {
        ao_values_.erase(ao_values_.begin());
    }
    
    // Calculate AccDec = AO - SMA(AO, period)
    if (ao_values_.size() >= params.period) {
        double sum_ao = 0.0;
        for (int i = 0; i < params.period; ++i) {
            sum_ao += ao_values_[ao_values_.size() - 1 - i];
        }
        double ao_sma = sum_ao / params.period;
        
        accde_line->set(0, ao_value - ao_sma);
    } else {
        accde_line->set(0, std::numeric_limits<double>::quiet_NaN());
    }
}

void AccelerationDecelerationOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    auto accde_line = lines->getline(accde);
    
    if (!high_line || !low_line || !accde_line) return;
    
    // Build median price array for all data
    std::vector<double> all_median_prices;
    for (int i = 0; i < end; ++i) {
        double high_value = (*high_line)[i];
        double low_value = (*low_line)[i];
        double median_price = (high_value + low_value) / 2.0;
        all_median_prices.push_back(median_price);
    }
    
    // Calculate AO values for all bars
    std::vector<double> all_ao_values;
    for (int i = 0; i < end; ++i) {
        if (i < 33) { // Need 34 bars for slow SMA
            all_ao_values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Calculate AO = SMA(5) - SMA(34) of median prices
            double sum_fast = 0.0, sum_slow = 0.0;
            
            for (int j = 0; j < 5; ++j) {
                sum_fast += all_median_prices[i - j];
            }
            double sma_fast = sum_fast / 5.0;
            
            for (int j = 0; j < 34; ++j) {
                sum_slow += all_median_prices[i - j];
            }
            double sma_slow = sum_slow / 34.0;
            
            all_ao_values.push_back(sma_fast - sma_slow);
        }
    }
    
    // Calculate AccDec for each bar from start to end
    for (int i = start; i < end; ++i) {
        if (i < 33 + params.period - 1) { // Need AO plus SMA period
            accde_line->set(i, std::numeric_limits<double>::quiet_NaN());
        } else {
            double ao_value = all_ao_values[i];
            
            // Calculate SMA of AO
            double sum_ao = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum_ao += all_ao_values[i - j];
            }
            double ao_sma = sum_ao / params.period;
            
            // AccDec = AO - SMA(AO)
            accde_line->set(i, ao_value - ao_sma);
        }
    }
}

} // namespace backtrader