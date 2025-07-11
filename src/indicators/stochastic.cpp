#include "indicators/stochastic.h"
#include "functions.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace backtrader {

// StochasticBase implementation
StochasticBase::StochasticBase() : Indicator() {
    _minperiod(params.period);
}

void StochasticBase::prenext() {
    Indicator::prenext();
}

void StochasticBase::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    calculate_lines();
}

void StochasticBase::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    for (int i = start; i < end; ++i) {
        calculate_lines();
    }
}

double StochasticBase::get_highest(int period, int offset) {
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

double StochasticBase::get_lowest(int period, int offset) {
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

// StochasticFast implementation
StochasticFast::StochasticFast() : StochasticBase() {
    setup_lines();
    
    // Create SMA for %D calculation
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    
    _minperiod(params.period + params.period_dfast - 1);
}

void StochasticFast::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void StochasticFast::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto close_line = datas[0]->lines->getline(3); // Close line
    auto k_line = lines->getline(percK);
    auto d_line = lines->getline(percD);
    
    if (!close_line || !k_line || !d_line) return;
    
    // Calculate %K
    double highest_high = get_highest(params.period);
    double lowest_low = get_lowest(params.period);
    double current_close = (*close_line)[0];
    
    double k_value = 0.0;
    if (highest_high != lowest_low) {
        k_value = 100.0 * (current_close - lowest_low) / (highest_high - lowest_low);
    } else if (params.safediv) {
        k_value = params.safezero;
    }
    
    k_line->set(0, k_value);
    
    // Store k value for SMA calculation
    k_values_.push_back(k_value);
    if (k_values_.size() > params.period_dfast) {
        k_values_.erase(k_values_.begin());
    }
    
    // Calculate %D as SMA of %K
    if (k_values_.size() >= params.period_dfast) {
        double sum = 0.0;
        for (double val : k_values_) {
            sum += val;
        }
        d_line->set(0, sum / params.period_dfast);
    }
}

// Stochastic implementation
Stochastic::Stochastic() : StochasticBase() {
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
}

Stochastic::Stochastic(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low, std::shared_ptr<LineRoot> close) 
    : StochasticBase() {
    // Use default parameters
    params.period = 14;
    params.period_dfast = 3;
    params.period_dslow = 3;
    
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
    
    // Add data sources
    if (high) {
        auto high_series = std::dynamic_pointer_cast<LineSeries>(high);
        if (high_series) datas.push_back(high_series);
    }
    if (low) {
        auto low_series = std::dynamic_pointer_cast<LineSeries>(low);
        if (low_series) datas.push_back(low_series);
    }
    if (close) {
        auto close_series = std::dynamic_pointer_cast<LineSeries>(close);
        if (close_series) datas.push_back(close_series);
    }
}

Stochastic::Stochastic(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low, std::shared_ptr<LineRoot> close, 
                       int period, int period_dfast) : StochasticBase() {
    params.period = period;
    params.period_dfast = period_dfast;
    params.period_dslow = 3; // Use default value for period_dslow
    
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
    
    // Add data sources
    if (high) {
        auto high_series = std::dynamic_pointer_cast<LineSeries>(high);
        if (high_series) datas.push_back(high_series);
    }
    if (low) {
        auto low_series = std::dynamic_pointer_cast<LineSeries>(low);
        if (low_series) datas.push_back(low_series);
    }
    if (close) {
        auto close_series = std::dynamic_pointer_cast<LineSeries>(close);
        if (close_series) datas.push_back(close_series);
    }
}

Stochastic::Stochastic(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low, std::shared_ptr<LineRoot> close, 
                       int period, int period_dfast, int period_dslow) : StochasticBase() {
    params.period = period;
    params.period_dfast = period_dfast;
    params.period_dslow = period_dslow;
    
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
    
    // Add data sources
    if (high) {
        auto high_series = std::dynamic_pointer_cast<LineSeries>(high);
        if (high_series) datas.push_back(high_series);
    }
    if (low) {
        auto low_series = std::dynamic_pointer_cast<LineSeries>(low);
        if (low_series) datas.push_back(low_series);
    }
    if (close) {
        auto close_series = std::dynamic_pointer_cast<LineSeries>(close);
        if (close_series) datas.push_back(close_series);
    }
}

void Stochastic::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Stochastic::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto close_line = datas[0]->lines->getline(3); // Close line
    auto k_line = lines->getline(percK);
    auto d_line = lines->getline(percD);
    
    if (!close_line || !k_line || !d_line) return;
    
    // Calculate raw %K
    double highest_high = get_highest(params.period);
    double lowest_low = get_lowest(params.period);
    double current_close = (*close_line)[0];
    
    double raw_k = 0.0;
    if (highest_high != lowest_low) {
        raw_k = 100.0 * (current_close - lowest_low) / (highest_high - lowest_low);
    } else if (params.safediv) {
        raw_k = params.safezero;
    }
    
    // Store raw k value for SMA calculation
    k_values_.push_back(raw_k);
    if (k_values_.size() > params.period_dfast) {
        k_values_.erase(k_values_.begin());
    }
    
    // Calculate %D as SMA of raw %K (this becomes %K in slow stochastic)
    double d_value = 0.0;
    if (k_values_.size() >= params.period_dfast) {
        double sum = 0.0;
        for (double val : k_values_) {
            sum += val;
        }
        d_value = sum / params.period_dfast;
    }
    
    k_line->set(0, d_value); // In slow stochastic, %K is the smoothed value
    
    // Store d value for slow %D calculation
    d_values_.push_back(d_value);
    if (d_values_.size() > params.period_dslow) {
        d_values_.erase(d_values_.begin());
    }
    
    // Calculate slow %D as SMA of %D
    if (d_values_.size() >= params.period_dslow) {
        double sum = 0.0;
        for (double val : d_values_) {
            sum += val;
        }
        d_line->set(0, sum / params.period_dslow);
    }
}

// StochasticFull implementation
StochasticFull::StochasticFull() : StochasticBase() {
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
}

StochasticFull::StochasticFull(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low, std::shared_ptr<LineRoot> close) 
    : StochasticBase() {
    // Use default parameters
    params.period = 14;
    params.period_dfast = 3;
    params.period_dslow = 3;
    
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
    
    // Add data sources
    if (high) {
        auto high_series = std::dynamic_pointer_cast<LineSeries>(high);
        if (high_series) datas.push_back(high_series);
    }
    if (low) {
        auto low_series = std::dynamic_pointer_cast<LineSeries>(low);
        if (low_series) datas.push_back(low_series);
    }
    if (close) {
        auto close_series = std::dynamic_pointer_cast<LineSeries>(close);
        if (close_series) datas.push_back(close_series);
    }
}

StochasticFull::StochasticFull(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low, std::shared_ptr<LineRoot> close, 
                               int period, int period_dfast, int period_dslow) : StochasticBase() {
    params.period = period;
    params.period_dfast = period_dfast;
    params.period_dslow = period_dslow;
    
    setup_lines();
    
    // Create SMA for %D and %DSlow calculations
    sma_fast_ = std::make_shared<indicators::SMA>(params.period_dfast);
    sma_dslow_ = std::make_shared<indicators::SMA>(params.period_dslow);
    
    _minperiod(params.period + params.period_dfast + params.period_dslow - 2);
    
    // Add data sources
    if (high) {
        auto high_series = std::dynamic_pointer_cast<LineSeries>(high);
        if (high_series) datas.push_back(high_series);
    }
    if (low) {
        auto low_series = std::dynamic_pointer_cast<LineSeries>(low);
        if (low_series) datas.push_back(low_series);
    }
    if (close) {
        auto close_series = std::dynamic_pointer_cast<LineSeries>(close);
        if (close_series) datas.push_back(close_series);
    }
}

void StochasticFull::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void StochasticFull::calculate_lines() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto close_line = datas[0]->lines->getline(3); // Close line
    auto k_line = lines->getline(percK);
    auto d_line = lines->getline(percD);
    auto dslow_line = lines->getline(percDSlow);
    
    if (!close_line || !k_line || !d_line || !dslow_line) return;
    
    // Calculate raw %K
    double highest_high = get_highest(params.period);
    double lowest_low = get_lowest(params.period);
    double current_close = (*close_line)[0];
    
    double raw_k = 0.0;
    if (highest_high != lowest_low) {
        raw_k = 100.0 * (current_close - lowest_low) / (highest_high - lowest_low);
    } else if (params.safediv) {
        raw_k = params.safezero;
    }
    
    k_line->set(0, raw_k);
    
    // Store k value for %D calculation
    k_values_.push_back(raw_k);
    if (k_values_.size() > params.period_dfast) {
        k_values_.erase(k_values_.begin());
    }
    
    // Calculate %D as SMA of %K
    double d_value = 0.0;
    if (k_values_.size() >= params.period_dfast) {
        double sum = 0.0;
        for (double val : k_values_) {
            sum += val;
        }
        d_value = sum / params.period_dfast;
    }
    
    d_line->set(0, d_value);
    
    // Store d value for %DSlow calculation
    d_values_.push_back(d_value);
    if (d_values_.size() > params.period_dslow) {
        d_values_.erase(d_values_.begin());
    }
    
    // Calculate %DSlow as SMA of %D
    if (d_values_.size() >= params.period_dslow) {
        double sum = 0.0;
        for (double val : d_values_) {
            sum += val;
        }
        dslow_line->set(0, sum / params.period_dslow);
    }
}

// StochasticBase utility methods for test framework compatibility
double StochasticBase::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto k_line = lines->getline(percK);
    if (!k_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*k_line)[ago];
}

int StochasticBase::getMinPeriod() const {
    return params.period;
}

void StochasticBase::calculate() {
    next();
}

double StochasticBase::getPercentK(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto k_line = lines->getline(percK);
    if (!k_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*k_line)[ago];
}

double StochasticBase::getPercentD(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto d_line = lines->getline(percD);
    if (!d_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*d_line)[ago];
}

} // namespace backtrader