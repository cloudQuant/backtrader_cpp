#include "analyzers/returns.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace backtrader {

Returns::Returns() : TimeFrameAnalyzerBase(), last_value_(0.0), first_value_(true), period_count_(0) {
    // Initialize with default parameters
}

void Returns::start() {
    TimeFrameAnalyzerBase::start();
    
    // Reset state
    results_ = Results{};
    returns_.clear();
    last_value_ = 0.0;
    first_value_ = true;
    period_count_ = 0;
}

void Returns::next() {
    // Get current portfolio value
    double current_value = 0.0;
    
    if (strategy_) {
        current_value = strategy_->broker->get_value();
    }
    
    if (first_value_) {
        last_value_ = current_value;
        first_value_ = false;
        return;
    }
    
    // Calculate logarithmic return
    if (last_value_ > 0.0 && current_value > 0.0) {
        double log_return = calculate_log_return(current_value, last_value_);
        returns_.push_back(log_return);
        period_count_++;
    }
    
    last_value_ = current_value;
}

void Returns::stop() {
    // Calculate final results
    calculate_final_results();
    
    TimeFrameAnalyzerBase::stop();
}

std::map<std::string, double> Returns::get_analysis() {
    std::map<std::string, double> analysis;
    
    analysis["rtot"] = results_.rtot;
    analysis["ravg"] = results_.ravg;
    analysis["rnorm"] = results_.rnorm;
    analysis["rnorm100"] = results_.rnorm100;
    
    return analysis;
}

double Returns::calculate_log_return(double current_value, double previous_value) {
    if (previous_value <= 0.0 || current_value <= 0.0) {
        return 0.0;
    }
    
    return std::log(current_value / previous_value);
}

void Returns::calculate_final_results() {
    if (returns_.empty()) {
        return;
    }
    
    // Total return (sum of log returns)
    results_.rtot = std::accumulate(returns_.begin(), returns_.end(), 0.0);
    
    // Average return
    results_.ravg = results_.rtot / returns_.size();
    
    // Annualized (normalized) return
    double ann_factor = get_annualization_factor();
    if (ann_factor > 0.0) {
        results_.rnorm = results_.ravg * ann_factor;
        results_.rnorm100 = results_.rnorm * 100.0;
    } else {
        results_.rnorm = results_.ravg;
        results_.rnorm100 = results_.rnorm * 100.0;
    }
}

double Returns::get_annualization_factor() {
    if (params.tann > 0.0) {
        return params.tann;
    }
    
    // Default annualization factors based on timeframe
    switch (params.timeframe) {
        case 1: // Days
            return 252.0;
        case 2: // Weeks
            return 52.0;
        case 3: // Months
            return 12.0;
        case 4: // Years
            return 1.0;
        default:
            // Try to infer from data
            return 252.0; // Default to daily
    }
}

} // namespace backtrader