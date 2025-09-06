#include "../../include/analyzers/vwr.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace backtrader {
namespace analyzers {

VWR::VWR(const Params& params) : p(params) {
    returns_.clear();
}

void VWR::start() {
    // Initialize analyzer
    returns_.clear();
    initial_value_ = 0.0;
    previous_value_ = 0.0;
    value_initialized_ = false;
}

void VWR::stop() {
    // Calculate final VWR
    calculate_vwr();
}

void VWR::prenext() {
    // Called before next() during minimum period
    next();
}

void VWR::next() {
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
        return;
    }
    
    // Calculate return for this period
    double period_return = 0.0;
    if (previous_value_ > 0.0) {
        period_return = (current_value - previous_value_) / previous_value_;
    }
    
    returns_.push_back(period_return);
    previous_value_ = current_value;
}

std::map<std::string, std::any> VWR::get_analysis() {
    calculate_vwr();
    
    std::map<std::string, std::any> analysis;
    
    analysis["vwr"] = vwr_;
    analysis["variability"] = variability_;
    analysis["geometric_mean_return"] = geometric_mean_return_;
    analysis["arithmetic_mean_return"] = arithmetic_mean_return_;
    analysis["std_dev"] = std_dev_;
    analysis["periods"] = static_cast<int>(returns_.size());
    analysis["annualized_return"] = annualized_return_;
    analysis["annualized_variability"] = annualized_variability_;
    analysis["annualized_vwr"] = annualized_vwr_;
    
    // Additional statistics
    analysis["sharpe_ratio"] = sharpe_ratio_;
    analysis["downside_deviation"] = downside_deviation_;
    analysis["max_return"] = max_return_;
    analysis["min_return"] = min_return_;
    analysis["positive_periods"] = positive_periods_;
    analysis["negative_periods"] = negative_periods_;
    analysis["win_rate"] = win_rate_;
    
    return analysis;
}

void VWR::calculate_vwr() {
    if (returns_.size() < p.periods) {
        vwr_ = 0.0;
        return;
    }
    
    // Calculate geometric mean return (GMR)
    geometric_mean_return_ = calculate_geometric_mean(returns_);
    
    // Calculate arithmetic mean return
    arithmetic_mean_return_ = calculate_arithmetic_mean(returns_);
    
    // Calculate standard deviation
    std_dev_ = calculate_std_dev(returns_, arithmetic_mean_return_);
    
    // Calculate variability
    variability_ = calculate_variability(returns_);
    
    // Calculate VWR = GMR / Variability
    if (variability_ > 0.0) {
        vwr_ = geometric_mean_return_ / variability_;
    } else {
        vwr_ = 0.0;
    }
    
    // Calculate annualized metrics
    double annualization_factor = get_annualization_factor();
    annualized_return_ = std::pow(1.0 + geometric_mean_return_, annualization_factor) - 1.0;
    annualized_variability_ = variability_ * std::sqrt(annualization_factor);
    
    if (annualized_variability_ > 0.0) {
        annualized_vwr_ = annualized_return_ / annualized_variability_;
    } else {
        annualized_vwr_ = 0.0;
    }
    
    // Calculate additional statistics
    calculate_additional_stats();
}

double VWR::calculate_geometric_mean(const std::vector<double>& returns) const {
    if (returns.empty()) {
        return 0.0;
    }
    
    double product = 1.0;
    for (double ret : returns) {
        product *= (1.0 + ret);
    }
    
    return std::pow(product, 1.0 / returns.size()) - 1.0;
}

double VWR::calculate_arithmetic_mean(const std::vector<double>& returns) const {
    if (returns.empty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(returns.begin(), returns.end(), 0.0);
    return sum / returns.size();
}

double VWR::calculate_std_dev(const std::vector<double>& returns, double mean) const {
    if (returns.size() < 2) {
        return 0.0;
    }
    
    double sum_squared_diff = 0.0;
    for (double ret : returns) {
        double diff = ret - mean;
        sum_squared_diff += diff * diff;
    }
    
    return std::sqrt(sum_squared_diff / (returns.size() - 1));
}

double VWR::calculate_variability(const std::vector<double>& returns) const {
    if (returns.size() < p.periods) {
        return 0.0;
    }
    
    // Calculate Variability as the standard deviation of log returns
    std::vector<double> log_returns;
    log_returns.reserve(returns.size());
    
    for (double ret : returns) {
        if (ret > -1.0) {
            log_returns.push_back(std::log(1.0 + ret));
        } else {
            // Handle extreme negative returns
            log_returns.push_back(-10.0); // Large negative value
        }
    }
    
    double mean_log_return = calculate_arithmetic_mean(log_returns);
    
    double sum_squared_diff = 0.0;
    for (double log_ret : log_returns) {
        double diff = log_ret - mean_log_return;
        sum_squared_diff += diff * diff;
    }
    
    // Use sample standard deviation
    return std::sqrt(sum_squared_diff / (log_returns.size() - 1));
}

void VWR::calculate_additional_stats() {
    if (returns_.empty()) {
        return;
    }
    
    // Min/Max returns
    auto minmax = std::minmax_element(returns_.begin(), returns_.end());
    min_return_ = *minmax.first;
    max_return_ = *minmax.second;
    
    // Count positive/negative periods
    positive_periods_ = 0;
    negative_periods_ = 0;
    
    for (double ret : returns_) {
        if (ret > 0.0) {
            positive_periods_++;
        } else if (ret < 0.0) {
            negative_periods_++;
        }
    }
    
    win_rate_ = static_cast<double>(positive_periods_) / returns_.size();
    
    // Calculate downside deviation
    downside_deviation_ = calculate_downside_deviation(returns_);
    
    // Calculate Sharpe ratio
    double annualization_factor = get_annualization_factor();
    double excess_return = annualized_return_ - p.risk_free_rate;
    double annualized_std = std_dev_ * std::sqrt(annualization_factor);
    
    if (annualized_std > 0.0) {
        sharpe_ratio_ = excess_return / annualized_std;
    } else {
        sharpe_ratio_ = 0.0;
    }
}

double VWR::calculate_downside_deviation(const std::vector<double>& returns) const {
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

double VWR::get_annualization_factor() const {
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

double VWR::get_vwr() const {
    return vwr_;
}

double VWR::get_annualized_vwr() const {
    return annualized_vwr_;
}

double VWR::get_sharpe_ratio() const {
    return sharpe_ratio_;
}

std::vector<double> VWR::get_returns() const {
    return returns_;
}

std::map<std::string, double> VWR::get_return_distribution() const {
    std::map<std::string, double> distribution;
    
    if (returns_.empty()) {
        return distribution;
    }
    
    // Calculate percentiles
    std::vector<double> sorted_returns = returns_;
    std::sort(sorted_returns.begin(), sorted_returns.end());
    
    size_t n = sorted_returns.size();
    
    // 5th percentile
    size_t idx_5 = static_cast<size_t>(n * 0.05);
    distribution["percentile_5"] = sorted_returns[idx_5];
    
    // 25th percentile (Q1)
    size_t idx_25 = static_cast<size_t>(n * 0.25);
    distribution["percentile_25"] = sorted_returns[idx_25];
    
    // 50th percentile (Median)
    size_t idx_50 = static_cast<size_t>(n * 0.50);
    distribution["median"] = sorted_returns[idx_50];
    
    // 75th percentile (Q3)
    size_t idx_75 = static_cast<size_t>(n * 0.75);
    distribution["percentile_75"] = sorted_returns[idx_75];
    
    // 95th percentile
    size_t idx_95 = static_cast<size_t>(n * 0.95);
    distribution["percentile_95"] = sorted_returns[idx_95];
    
    // Interquartile range
    distribution["iqr"] = sorted_returns[idx_75] - sorted_returns[idx_25];
    
    // Calculate skewness
    double skewness = calculate_skewness(returns_, arithmetic_mean_return_, std_dev_);
    distribution["skewness"] = skewness;
    
    // Calculate kurtosis
    double kurtosis = calculate_kurtosis(returns_, arithmetic_mean_return_, std_dev_);
    distribution["kurtosis"] = kurtosis;
    
    return distribution;
}

double VWR::calculate_skewness(const std::vector<double>& returns, 
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

double VWR::calculate_kurtosis(const std::vector<double>& returns, 
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

void VWR::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

} // namespace analyzers
} // namespace backtrader