#include "../../include/analyzers/logreturnsrolling.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <numeric>
#include <cmath>

namespace backtrader {
namespace analyzers {

LogReturnsRolling::LogReturnsRolling(const Params& params) 
    : TimeFrameAnalyzerBase(params), p(params) {
    
    // Initialize rolling window
    _values.clear();
    
    // Initialize state
    _value = 0.0;
    _lastvalue = 0.0;
    _fundmode = false;
    
    // Clear results
    rets.clear();
    
    // Invalidate stats cache
    invalidate_stats_cache();
}

void LogReturnsRolling::start() {
    // Call parent start
    TimeFrameAnalyzerBase::start();
    
    // Initialize fund mode detection
    _fundmode = (p.fund != nullptr);
    
    // Initialize rolling window with compression size
    initialize_rolling_window();
    
    // Clear results
    rets.clear();
    
    // Invalidate stats cache
    invalidate_stats_cache();
}

void LogReturnsRolling::next() {
    // Call parent next (handles timeframe logic)
    TimeFrameAnalyzerBase::next();
    
    // Update current value based on mode
    if (_fundmode) {
        _value = get_current_fund_value();
    } else if (p.data) {
        _value = get_current_data_value();
    } else {
        _value = get_current_portfolio_value();
    }
    
    // Calculate log return if we have enough values in rolling window
    if (_values.size() >= static_cast<size_t>(compression)) {
        double reference_value = _values.front(); // Oldest value in window
        double log_return = calculate_log_return(_value, reference_value);
        
        // Store result with current datetime key
        if (datas.empty() || !datas[0]) {
            rets[std::chrono::system_clock::now()] = log_return;
        } else {
            // TODO: Get actual datetime from data
            // In real implementation: rets[datas[0]->datetime.datetime()] = log_return;
            rets[std::chrono::system_clock::now()] = log_return;
        }
        
        // Invalidate stats cache
        invalidate_stats_cache();
    }
}

void LogReturnsRolling::on_dt_over() {
    // Called when crossing timeframe boundaries
    // Update rolling window with current value
    update_rolling_window(_value);
    
    // Store last value for reference
    _lastvalue = _value;
}

void LogReturnsRolling::notify_fund(double cash, double value, double fundvalue, double shares) {
    // Call parent notification
    TimeFrameAnalyzerBase::notify_fund(cash, value, fundvalue, shares);
    
    // Update current value in fund mode
    if (_fundmode) {
        _value = fundvalue;
    }
}

AnalysisResult LogReturnsRolling::get_analysis() const {
    AnalysisResult result;
    
    // Convert OrderedDict to AnalysisResult format
    std::map<std::string, double> datetime_return_map;
    
    for (const auto& pair : rets) {
        std::string datetime_str = format_datetime(pair.first);
        datetime_return_map[datetime_str] = pair.second;
    }
    
    // Store as a map in the result
    result["returns"] = datetime_return_map;
    
    // Add summary statistics
    if (!rets.empty()) {
        result["mean_return"] = get_mean_return();
        result["std_return"] = get_std_return();
        result["sharpe_ratio"] = get_sharpe_ratio();
        result["count"] = static_cast<int>(rets.size());
        result["window_size"] = static_cast<int>(compression);
    }
    
    return result;
}

std::vector<double> LogReturnsRolling::get_returns_history() const {
    std::vector<double> returns;
    returns.reserve(rets.size());
    
    for (const auto& pair : rets) {
        returns.push_back(pair.second);
    }
    
    return returns;
}

double LogReturnsRolling::get_mean_return() const {
    update_stats_cache();
    return cached_mean_return;
}

double LogReturnsRolling::get_std_return() const {
    update_stats_cache();
    return cached_std_return;
}

double LogReturnsRolling::get_sharpe_ratio() const {
    update_stats_cache();
    return cached_sharpe_ratio;
}

void LogReturnsRolling::export_to_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    // Write header
    file << "datetime,log_return\n";
    
    // Write data
    for (const auto& pair : rets) {
        file << format_datetime(pair.first) << "," << std::fixed << std::setprecision(8) << pair.second << "\n";
    }
    
    file.close();
}

double LogReturnsRolling::get_current_portfolio_value() const {
    // Access portfolio value through strategy
    if (!strategy) {
        return 0.0;
    }
    
    // TODO: Implement actual broker access
    // return strategy->broker->getvalue();
    
    // Placeholder implementation
    static double dummy_value = 100000.0;
    dummy_value += (rand() % 2001 - 1000) * 0.01; // Random walk
    return dummy_value;
}

double LogReturnsRolling::get_current_data_value() const {
    // Access data value from specified data feed
    if (!p.data) {
        return 0.0;
    }
    
    // TODO: Implement actual data access
    // if (p.firstopen) {
    //     return p.data->open[0];
    // } else {
    //     return p.data->close[0];
    // }
    
    // Placeholder implementation
    static double dummy_price = 100.0;
    dummy_price += (rand() % 201 - 100) * 0.01; // Random walk
    return dummy_price;
}

double LogReturnsRolling::get_current_fund_value() const {
    // Access fund value
    if (!p.fund) {
        return 0.0;
    }
    
    // TODO: Implement actual fund access
    // return p.fund->fundvalue[0];
    
    // Placeholder implementation
    static double dummy_fund_value = 1000000.0;
    dummy_fund_value += (rand() % 10001 - 5000) * 0.01; // Random walk
    return dummy_fund_value;
}

double LogReturnsRolling::calculate_log_return(double current_value, double reference_value) const {
    // Calculate logarithmic return with error handling
    if (!is_valid_for_log(current_value) || !is_valid_for_log(reference_value)) {
        return 0.0;
    }
    
    if (reference_value == 0.0) {
        return 0.0;
    }
    
    try {
        double ratio = current_value / reference_value;
        if (ratio <= 0.0) {
            // Handle negative or zero ratio (大亏损情况处理)
            return 0.0;
        }
        
        return std::log(ratio);
    } catch (const std::exception& e) {
        // Handle any mathematical errors
        return 0.0;
    }
}

bool LogReturnsRolling::is_valid_for_log(double value) const {
    // Check if value is valid for logarithm calculation
    return std::isfinite(value) && value > 0.0;
}

void LogReturnsRolling::update_rolling_window(double new_value) {
    // Add new value to rolling window
    _values.push_back(new_value);
    
    // Maintain window size (deque automatically handles maxlen behavior)
    while (_values.size() > static_cast<size_t>(compression)) {
        _values.pop_front();
    }
}

void LogReturnsRolling::initialize_rolling_window() {
    // Initialize rolling window with compression size
    _values.clear();
    
    // The window will be populated as data arrives
    // Maximum size will be maintained by update_rolling_window
}

std::string LogReturnsRolling::format_datetime(const std::chrono::system_clock::time_point& dt) const {
    // Convert time_point to time_t
    std::time_t time_t = std::chrono::system_clock::to_time_t(dt);
    
    // Convert to tm struct
    std::tm* tm_ptr = std::localtime(&time_t);
    
    // Format as ISO string
    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    
    return oss.str();
}

void LogReturnsRolling::invalidate_stats_cache() {
    stats_cached = false;
}

void LogReturnsRolling::update_stats_cache() const {
    if (stats_cached || rets.empty()) {
        return;
    }
    
    // Get returns history
    auto returns = get_returns_history();
    
    if (returns.empty()) {
        cached_mean_return = 0.0;
        cached_std_return = 0.0;
        cached_sharpe_ratio = 0.0;
        stats_cached = true;
        return;
    }
    
    // Calculate mean
    double sum = std::accumulate(returns.begin(), returns.end(), 0.0);
    cached_mean_return = sum / returns.size();
    
    // Calculate standard deviation
    double sq_sum = std::inner_product(returns.begin(), returns.end(), returns.begin(), 0.0);
    cached_std_return = std::sqrt(sq_sum / returns.size() - cached_mean_return * cached_mean_return);
    
    // Calculate Sharpe ratio (assuming zero risk-free rate)
    if (cached_std_return != 0.0) {
        cached_sharpe_ratio = cached_mean_return / cached_std_return;
    } else {
        cached_sharpe_ratio = 0.0;
    }
    
    stats_cached = true;
}

} // namespace analyzers
} // namespace backtrader