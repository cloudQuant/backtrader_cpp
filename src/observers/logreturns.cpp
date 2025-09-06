#include "../../include/observers/logreturns.h"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace backtrader {
namespace observers {

LogReturns::LogReturns(const Params& params) 
    : p(params), initial_value_(0.0), previous_value_(0.0), 
      value_initialized_(false) {
    
    // Initialize lines
    lines.resize(3);
    lines[0].clear(); // log returns
    lines[1].clear(); // cumulative log returns
    lines[2].clear(); // annualized returns
    
    // Set line names
    line_names.resize(3);
    line_names[0] = "logreturns";
    line_names[1] = "cumulative";
    line_names[2] = "annualized";
}

void LogReturns::prenext() {
    // Called before next() during minimum period
    next();
}

void LogReturns::next() {
    if (!broker_) {
        return;
    }
    
    // Get current portfolio value
    double current_value = broker_->get_value();
    
    // Initialize on first call
    if (!value_initialized_) {
        initial_value_ = current_value;
        previous_value_ = current_value;
        value_initialized_ = true;
        
        // First bar has zero return
        lines[0].push_back(0.0);
        lines[1].push_back(0.0);
        lines[2].push_back(0.0);
        return;
    }
    
    // Calculate log return
    double log_return = 0.0;
    if (previous_value_ > 0.0 && current_value > 0.0) {
        log_return = std::log(current_value / previous_value_);
    }
    
    // Store log return
    lines[0].push_back(log_return);
    
    // Calculate cumulative log return
    double cumulative_log_return = 0.0;
    if (initial_value_ > 0.0 && current_value > 0.0) {
        cumulative_log_return = std::log(current_value / initial_value_);
    }
    lines[1].push_back(cumulative_log_return);
    
    // Calculate annualized return
    double annualized_return = calculate_annualized_return(cumulative_log_return);
    lines[2].push_back(annualized_return);
    
    // Update previous value
    previous_value_ = current_value;
}

double LogReturns::calculate_annualized_return(double cumulative_log_return) const {
    int periods = static_cast<int>(lines[0].size());
    if (periods <= 0) {
        return 0.0;
    }
    
    // Get annualization factor based on timeframe
    double annualization_factor = get_annualization_factor();
    
    // Annualized return = (exp(cumulative_log_return) - 1) * (annualization_factor / periods)
    if (periods > 0) {
        double total_return = std::exp(cumulative_log_return) - 1.0;
        double years = periods / annualization_factor;
        
        if (years > 0.0) {
            // Compound annual growth rate
            return std::pow(1.0 + total_return, 1.0 / years) - 1.0;
        }
    }
    
    return 0.0;
}

double LogReturns::get_annualization_factor() const {
    // Return number of periods in a year based on timeframe
    switch (p.timeframe) {
        case 1: // Daily
            return 252.0; // Trading days
        case 2: // Weekly
            return 52.0;
        case 3: // Monthly
            return 12.0;
        case 4: // Quarterly
            return 4.0;
        case 5: // Yearly
            return 1.0;
        default:
            return 252.0; // Default to daily
    }
}

double LogReturns::get_current_return() const {
    if (!lines[0].empty()) {
        return lines[0].back();
    }
    return 0.0;
}

double LogReturns::get_cumulative_return() const {
    if (!lines[1].empty()) {
        return std::exp(lines[1].back()) - 1.0; // Convert from log to regular return
    }
    return 0.0;
}

double LogReturns::get_annualized_return() const {
    if (!lines[2].empty()) {
        return lines[2].back();
    }
    return 0.0;
}

std::map<std::string, double> LogReturns::get_analysis() const {
    std::map<std::string, double> analysis;
    
    if (lines[0].empty()) {
        return analysis;
    }
    
    const auto& log_returns = lines[0];
    
    // Basic statistics
    analysis["total_periods"] = static_cast<double>(log_returns.size());
    analysis["cumulative_return"] = get_cumulative_return();
    analysis["annualized_return"] = get_annualized_return();
    
    // Calculate mean log return
    double sum = std::accumulate(log_returns.begin(), log_returns.end(), 0.0);
    double mean = sum / log_returns.size();
    analysis["mean_log_return"] = mean;
    analysis["mean_return"] = std::exp(mean) - 1.0;
    
    // Calculate standard deviation
    double sq_sum = 0.0;
    for (double ret : log_returns) {
        sq_sum += (ret - mean) * (ret - mean);
    }
    double std_dev = std::sqrt(sq_sum / log_returns.size());
    analysis["std_dev"] = std_dev;
    
    // Calculate annualized volatility
    double annualization_factor = get_annualization_factor();
    analysis["annualized_volatility"] = std_dev * std::sqrt(annualization_factor);
    
    // Calculate Sharpe ratio (assuming risk-free rate from params)
    double excess_return = get_annualized_return() - p.risk_free_rate;
    double annualized_vol = analysis["annualized_volatility"];
    if (annualized_vol > 0.0) {
        analysis["sharpe_ratio"] = excess_return / annualized_vol;
    } else {
        analysis["sharpe_ratio"] = 0.0;
    }
    
    // Calculate max and min returns
    auto minmax = std::minmax_element(log_returns.begin(), log_returns.end());
    analysis["min_log_return"] = *minmax.first;
    analysis["max_log_return"] = *minmax.second;
    analysis["min_return"] = std::exp(*minmax.first) - 1.0;
    analysis["max_return"] = std::exp(*minmax.second) - 1.0;
    
    // Count positive and negative returns
    int positive_returns = 0;
    int negative_returns = 0;
    int zero_returns = 0;
    
    for (double ret : log_returns) {
        if (ret > 0.0) {
            positive_returns++;
        } else if (ret < 0.0) {
            negative_returns++;
        } else {
            zero_returns++;
        }
    }
    
    analysis["positive_returns"] = static_cast<double>(positive_returns);
    analysis["negative_returns"] = static_cast<double>(negative_returns);
    analysis["zero_returns"] = static_cast<double>(zero_returns);
    analysis["win_rate"] = static_cast<double>(positive_returns) / log_returns.size();
    
    // Calculate skewness
    double skewness = calculate_skewness(log_returns, mean, std_dev);
    analysis["skewness"] = skewness;
    
    // Calculate kurtosis
    double kurtosis = calculate_kurtosis(log_returns, mean, std_dev);
    analysis["kurtosis"] = kurtosis;
    
    // Calculate downside deviation
    double downside_dev = calculate_downside_deviation(log_returns);
    analysis["downside_deviation"] = downside_dev;
    
    // Calculate Sortino ratio
    if (downside_dev > 0.0) {
        analysis["sortino_ratio"] = excess_return / (downside_dev * std::sqrt(annualization_factor));
    } else {
        analysis["sortino_ratio"] = 0.0;
    }
    
    return analysis;
}

double LogReturns::calculate_skewness(const std::vector<double>& returns, 
                                     double mean, double std_dev) const {
    if (returns.size() < 3 || std_dev == 0.0) {
        return 0.0;
    }
    
    double n = static_cast<double>(returns.size());
    double sum_cubed = 0.0;
    
    for (double ret : returns) {
        double diff = ret - mean;
        sum_cubed += diff * diff * diff;
    }
    
    double skewness = (n / ((n - 1.0) * (n - 2.0))) * 
                      (sum_cubed / (std_dev * std_dev * std_dev));
    
    return skewness;
}

double LogReturns::calculate_kurtosis(const std::vector<double>& returns, 
                                    double mean, double std_dev) const {
    if (returns.size() < 4 || std_dev == 0.0) {
        return 0.0;
    }
    
    double n = static_cast<double>(returns.size());
    double sum_fourth = 0.0;
    
    for (double ret : returns) {
        double diff = ret - mean;
        sum_fourth += diff * diff * diff * diff;
    }
    
    double kurtosis = (n * (n + 1.0) / ((n - 1.0) * (n - 2.0) * (n - 3.0))) * 
                      (sum_fourth / (std_dev * std_dev * std_dev * std_dev)) -
                      (3.0 * (n - 1.0) * (n - 1.0) / ((n - 2.0) * (n - 3.0)));
    
    return kurtosis;
}

double LogReturns::calculate_downside_deviation(const std::vector<double>& returns) const {
    if (returns.empty()) {
        return 0.0;
    }
    
    double sum_squared = 0.0;
    int count = 0;
    
    for (double ret : returns) {
        if (ret < 0.0) {
            sum_squared += ret * ret;
            count++;
        }
    }
    
    if (count > 0) {
        return std::sqrt(sum_squared / count);
    }
    
    return 0.0;
}

void LogReturns::reset() {
    initial_value_ = 0.0;
    previous_value_ = 0.0;
    value_initialized_ = false;
    
    for (auto& line : lines) {
        line.clear();
    }
}

void LogReturns::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

std::vector<double> LogReturns::get_returns() const {
    std::vector<double> regular_returns;
    regular_returns.reserve(lines[0].size());
    
    for (double log_ret : lines[0]) {
        regular_returns.push_back(std::exp(log_ret) - 1.0);
    }
    
    return regular_returns;
}

std::vector<double> LogReturns::get_log_returns() const {
    return lines[0];
}

std::vector<double> LogReturns::get_cumulative_returns() const {
    std::vector<double> cum_returns;
    cum_returns.reserve(lines[1].size());
    
    for (double cum_log_ret : lines[1]) {
        cum_returns.push_back(std::exp(cum_log_ret) - 1.0);
    }
    
    return cum_returns;
}

} // namespace observers
} // namespace backtrader