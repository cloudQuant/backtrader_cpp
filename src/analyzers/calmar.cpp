#include "analyzers/calmar.h"
#include <algorithm>
#include <limits>
#include <sstream>
#include <iomanip>

namespace backtrader {

Calmar::Calmar() : TimeFrameAnalyzerBase(), max_dd_(std::numeric_limits<double>::lowest()), 
                   current_calmar_(0.0), fundmode_(false) {
    // Initialize with default parameters
}

void Calmar::start() {
    TimeFrameAnalyzerBase::start();
    
    // Auto-detect fund mode if needed
    if (params.auto_fund && strategy_) {
        fundmode_ = strategy_->broker->get_fundmode();
    } else {
        fundmode_ = params.fund;
    }
    
    // Initialize the maximum drawdown analyzer
    max_drawdown_ = std::make_unique<TimeDrawDown>();
    if (max_drawdown_) {
        max_drawdown_->params.timeframe = static_cast<int>(params.timeframe);
        max_drawdown_->params.compression = params.compression;
        max_drawdown_->params.fund = fundmode_;
        
        if (strategy_) {
            max_drawdown_->set_strategy(strategy_);
        }
        max_drawdown_->start();
    }
    
    // Initialize rolling values deque with NaN values
    values_.clear();
    for (int i = 0; i < params.period; ++i) {
        values_.push_back(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Initialize state
    max_dd_ = std::numeric_limits<double>::lowest();
    current_calmar_ = 0.0;
    results_.clear();
    
    // Add initial portfolio value
    if (strategy_) {
        double initial_value = fundmode_ ? 
            strategy_->broker->get_fundvalue() : 
            strategy_->broker->get_value();
        values_.push_back(initial_value);
    }
}

void Calmar::stop() {
    // Update final values
    on_dt_over();
    
    // Stop child analyzer
    if (max_drawdown_) {
        max_drawdown_->stop();
    }
    
    TimeFrameAnalyzerBase::stop();
}

void Calmar::on_dt_over() {
    // Update maximum drawdown from child analyzer
    if (max_drawdown_) {
        auto dd_analysis = max_drawdown_->get_analysis();
        auto it = dd_analysis.find("maxdrawdown");
        if (it != dd_analysis.end()) {
            max_dd_ = std::max(max_dd_, it->second);
        }
    }
    
    // Add current portfolio value to rolling window
    if (strategy_) {
        double current_value = fundmode_ ? 
            strategy_->broker->get_fundvalue() : 
            strategy_->broker->get_value();
        values_.push_back(current_value);
    }
    
    // Calculate annualized return and Calmar ratio
    double annualized_return = calculate_annualized_return();
    
    // Calculate Calmar ratio
    if (max_dd_ > 0.0) {
        current_calmar_ = annualized_return / (max_dd_ / 100.0); // Convert percentage to decimal
    } else if (max_dd_ == 0.0) {
        current_calmar_ = std::numeric_limits<double>::infinity();
    } else {
        current_calmar_ = 0.0;
    }
    
    // Store result with date key
    std::string date_key = get_current_date_key();
    results_[date_key] = current_calmar_;
}

std::map<std::string, double> Calmar::get_analysis() {
    return results_;
}

double Calmar::calculate_annualized_return() {
    // Need at least 2 valid values to calculate return
    if (values_.size() < 2) {
        return 0.0;
    }
    
    // Find first and last valid values
    double first_value = std::numeric_limits<double>::quiet_NaN();
    double last_value = std::numeric_limits<double>::quiet_NaN();
    
    // Find first valid value
    for (const double& value : values_) {
        if (!std::isnan(value)) {
            first_value = value;
            break;
        }
    }
    
    // Find last valid value
    for (auto it = values_.rbegin(); it != values_.rend(); ++it) {
        if (!std::isnan(*it)) {
            last_value = *it;
            break;
        }
    }
    
    // Calculate logarithmic return
    if (std::isnan(first_value) || std::isnan(last_value) || 
        first_value <= 0.0 || last_value <= 0.0) {
        return 0.0;
    }
    
    // Calculate annualized return using logarithmic approach
    // Default: rann = log(end_value / start_value) / period_length
    double log_return = std::log(last_value / first_value);
    
    // Count valid periods
    int valid_periods = 0;
    for (const double& value : values_) {
        if (!std::isnan(value)) {
            valid_periods++;
        }
    }
    
    if (valid_periods > 1) {
        return log_return / valid_periods;
    }
    
    return 0.0;
}

std::string Calmar::get_current_date_key() const {
    // Generate a date key for the current period
    // In a real implementation, this would use the actual datetime from the data
    static int period_counter = 0;
    period_counter++;
    
    std::stringstream ss;
    ss << "period_" << std::setfill('0') << std::setw(4) << period_counter;
    return ss.str();
}

} // namespace backtrader