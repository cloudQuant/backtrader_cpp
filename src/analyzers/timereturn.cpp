#include "analyzers/timereturn.h"
#include <sstream>
#include <iomanip>

namespace backtrader {

TimeReturn::TimeReturn() : TimeFrameAnalyzerBase(), value_start_(0.0), last_value_(0.0), 
                          current_value_(0.0), fundmode_(false) {
    // Initialize with default parameters
}

void TimeReturn::start() {
    TimeFrameAnalyzerBase::start();
    
    // Auto-detect fund mode if needed
    if (params.auto_fund && strategy_) {
        fundmode_ = strategy_->broker->get_fundmode();
    } else {
        fundmode_ = params.fund;
    }
    
    // Initialize values
    value_start_ = 0.0;
    last_value_ = 0.0;
    current_value_ = 0.0;
    returns_.clear();
    
    // Set initial portfolio value
    if (strategy_) {
        if (fundmode_) {
            last_value_ = strategy_->broker->get_fundvalue();
        } else {
            last_value_ = strategy_->broker->get_value();
        }
    }
}

void TimeReturn::next() {
    TimeFrameAnalyzerBase::next();
    
    // Calculate return for current period
    if (value_start_ > 0.0 && current_value_ > 0.0) {
        double return_value = (current_value_ / value_start_) - 1.0;
        std::string date_key = get_current_date_key();
        returns_[date_key] = return_value;
    }
    
    // Update last value
    last_value_ = current_value_;
}

void TimeReturn::notify_fund(double cash, double value, double fundvalue, int shares) {
    // Update current value based on fund mode
    if (fundmode_) {
        current_value_ = fundvalue;
    } else {
        current_value_ = value;
    }
}

void TimeReturn::on_dt_over() {
    // Called when timeframe period ends
    if (last_value_ > 0.0) {
        value_start_ = last_value_;
    } else {
        // First period - use current value as start
        value_start_ = current_value_;
    }
}

std::map<std::string, double> TimeReturn::get_analysis() {
    return returns_;
}

std::vector<double> TimeReturn::get_returns() const {
    std::vector<double> returns_vector;
    returns_vector.reserve(returns_.size());
    
    for (const auto& pair : returns_) {
        returns_vector.push_back(pair.second);
    }
    
    return returns_vector;
}

std::string TimeReturn::get_current_date_key() const {
    // Generate a simple date key for the current period
    // In a real implementation, this would use the actual datetime from the data
    static int period_counter = 0;
    period_counter++;
    
    std::stringstream ss;
    ss << "period_" << std::setfill('0') << std::setw(4) << period_counter;
    return ss.str();
}

} // namespace backtrader