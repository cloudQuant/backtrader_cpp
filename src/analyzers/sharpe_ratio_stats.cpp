#include "../../include/analyzers/sharpe_ratio_stats.h"
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

SharpeRatioStats::SharpeRatioStats(const Params& params) : p(params) {
    // Validate parameters
    validate_parameters();
    
    // Initialize data storage
    returns_.clear();
    dates_.clear();
    
    // Initialize state
    portfolio_value_start_ = 0.0;
    portfolio_value_previous_ = 0.0;
    
    // Invalidate cache
    invalidate_cache();
}

void SharpeRatioStats::start() {
    // Call parent start
    Analyzer::start();
    
    // Clear data
    returns_.clear();
    dates_.clear();
    
    // Initialize portfolio values
    portfolio_value_start_ = get_current_portfolio_value();
    portfolio_value_previous_ = portfolio_value_start_;
    
    // Invalidate cache
    invalidate_cache();
}

void SharpeRatioStats::next() {
    // Call parent next
    Analyzer::next();
    
    // Get current portfolio value
    double current_value = get_current_portfolio_value();
    
    // Calculate return if we have a previous value
    if (portfolio_value_previous_ > 0.0) {
        double return_value = (current_value - portfolio_value_previous_) / portfolio_value_previous_;
        
        // Validate and store return
        if (is_valid_return(return_value)) {
            returns_.push_back(return_value);
            dates_.push_back(get_current_datetime());
            
            // Invalidate cache
            invalidate_cache();
        }
    }
    
    // Update previous value
    portfolio_value_previous_ = current_value;
}

void SharpeRatioStats::stop() {
    // Call parent stop
    Analyzer::stop();
    
    // Ensure cache is updated
    update_cache();
}

AnalysisResult SharpeRatioStats::get_analysis() const {
    AnalysisResult result;
    
    // Get comprehensive statistics
    auto stats = get_sharpe_statistics();
    
    // Basic Sharpe metrics
    result["sharpe_ratio"] = stats.sharpe_ratio;
    result["annualized_sharpe"] = stats.annualized_sharpe;
    result["sharpe_std"] = stats.sharpe_std;
    
    // Advanced statistical metrics
    if (p.calculate_psr) {
        result["probabilistic_sharpe_ratio"] = stats.probabilistic_sharpe_ratio;
    }
    
    if (p.calculate_dsr) {
        result["deflated_sharpe_ratio"] = stats.deflated_sharpe_ratio;
    }
    
    if (p.calculate_min_trl) {
        result["minimum_track_record_length"] = stats.minimum_track_record_length;
    }
    
    // Statistical significance
    result["t_statistic"] = stats.t_statistic;
    result["p_value"] = stats.p_value;
    result["is_significant"] = stats.is_significant ? 1.0 : 0.0;
    
    // Confidence intervals
    if (p.calculate_confidence_intervals) {
        result["sharpe_lower_ci"] = stats.sharpe_lower_ci;
        result["sharpe_upper_ci"] = stats.sharpe_upper_ci;
    }
    
    // Sample statistics
    result["sample_size"] = static_cast<double>(stats.sample_size);
    result["mean_return"] = stats.mean_return;
    result["return_std"] = stats.return_std;
    result["skewness"] = stats.skewness;
    result["kurtosis"] = stats.kurtosis;
    
    // Risk metrics
    result["var_95"] = stats.var_95;
    result["cvar_95"] = stats.cvar_95;
    result["maximum_drawdown"] = stats.maximum_drawdown;
    
    // Additional ratios
    result["information_ratio"] = stats.information_ratio;
    result["treynor_ratio"] = stats.treynor_ratio;
    result["sortino_ratio"] = stats.sortino_ratio;
    
    return result;
}

SharpeRatioStats::SharpeStatistics SharpeRatioStats::get_sharpe_statistics() const {
    update_cache();
    return cached_stats_;
}

double SharpeRatioStats::get_sharpe_ratio() const {
    return calculate_sharpe_ratio();
}

double SharpeRatioStats::get_probabilistic_sharpe_ratio() const {
    return calculate_probabilistic_sharpe_ratio();
}

double SharpeRatioStats::get_deflated_sharpe_ratio() const {
    return calculate_deflated_sharpe_ratio();
}

double SharpeRatioStats::get_minimum_track_record_length() const {
    return calculate_minimum_track_record_length();
}

bool SharpeRatioStats::test_sharpe_significance() const {
    double p_val = calculate_p_value();
    return p_val < (1.0 - p.confidence_level);
}

std::pair<double, double> SharpeRatioStats::get_sharpe_confidence_interval() const {
    return calculate_confidence_interval();
}

double SharpeRatioStats::calculate_t_statistic() const {
    return calculate_t_statistic_internal();
}

double SharpeRatioStats::calculate_p_value() const {
    return calculate_p_value_internal();
}

void SharpeRatioStats::export_statistics_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    auto stats = get_sharpe_statistics();
    
    // Write statistics
    file << "metric,value\n";
    file << "sharpe_ratio," << std::fixed << std::setprecision(6) << stats.sharpe_ratio << "\n";
    file << "annualized_sharpe," << stats.annualized_sharpe << "\n";
    file << "sharpe_std," << stats.sharpe_std << "\n";
    file << "probabilistic_sharpe_ratio," << stats.probabilistic_sharpe_ratio << "\n";
    file << "deflated_sharpe_ratio," << stats.deflated_sharpe_ratio << "\n";
    file << "minimum_track_record_length," << stats.minimum_track_record_length << "\n";
    file << "t_statistic," << stats.t_statistic << "\n";
    file << "p_value," << stats.p_value << "\n";
    file << "is_significant," << (stats.is_significant ? 1 : 0) << "\n";
    file << "sharpe_lower_ci," << stats.sharpe_lower_ci << "\n";
    file << "sharpe_upper_ci," << stats.sharpe_upper_ci << "\n";
    file << "sample_size," << stats.sample_size << "\n";
    file << "mean_return," << stats.mean_return << "\n";
    file << "return_std," << stats.return_std << "\n";
    file << "skewness," << stats.skewness << "\n";
    file << "kurtosis," << stats.kurtosis << "\n";
    
    file.close();
}

void SharpeRatioStats::export_returns_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    // Write header
    file << "date,return\n";
    
    // Write data
    for (size_t i = 0; i < returns_.size() && i < dates_.size(); ++i) {
        file << format_datetime(dates_[i]) << "," 
             << std::fixed << std::setprecision(8) << returns_[i] << "\n";
    }
    
    file.close();
}

double SharpeRatioStats::calculate_sharpe_ratio() const {
    if (returns_.empty()) {
        return 0.0;
    }
    
    double mean_return = calculate_mean();
    double std_return = calculate_std();
    
    if (std_return == 0.0) {
        return 0.0;
    }
    
    return (mean_return - p.risk_free_rate) / std_return;
}

double SharpeRatioStats::calculate_annualized_sharpe() const {
    double sharpe = calculate_sharpe_ratio();
    return sharpe * std::sqrt(p.annualization_factor);
}

double SharpeRatioStats::calculate_sharpe_std() const {
    if (returns_.empty()) {
        return 0.0;
    }
    
    double sharpe = calculate_sharpe_ratio();
    double skew = calculate_skewness();
    double kurt = calculate_kurtosis();
    
    // Sharpe ratio standard error formula including skewness and kurtosis
    double n = static_cast<double>(returns_.size());
    double var_sharpe = (1.0 + 0.5 * sharpe * sharpe - skew * sharpe + (kurt - 3.0) * sharpe * sharpe / 4.0) / n;
    
    return std::sqrt(var_sharpe);
}

double SharpeRatioStats::calculate_probabilistic_sharpe_ratio() const {
    if (returns_.empty()) {
        return 0.0;
    }
    
    double sharpe = calculate_sharpe_ratio();
    double sharpe_std = calculate_sharpe_std();
    
    if (sharpe_std == 0.0) {
        return sharpe > p.benchmark_sharpe ? 1.0 : 0.0;
    }
    
    double z = (sharpe - p.benchmark_sharpe) / sharpe_std;
    return normal_cdf(z);
}

double SharpeRatioStats::calculate_deflated_sharpe_ratio() const {
    // Simplified DSR calculation - in practice would require more historical data
    double psr = calculate_probabilistic_sharpe_ratio();
    double expected_max = calculate_expected_max_sharpe();
    
    if (expected_max == 0.0) {
        return psr;
    }
    
    // Deflation factor based on multiple testing
    double deflation_factor = 1.0 - std::log(2.0) / std::log(static_cast<double>(returns_.size()));
    return psr * deflation_factor;
}

double SharpeRatioStats::calculate_minimum_track_record_length() const {
    if (returns_.empty()) {
        return 0.0;
    }
    
    double sharpe = calculate_sharpe_ratio();
    double sharpe_std = calculate_sharpe_std();
    
    if (sharpe_std == 0.0 || sharpe == 0.0) {
        return 0.0;
    }
    
    // Confidence level for statistical significance
    double z_alpha = -normal_cdf(1.0 - p.confidence_level);  // Approximation
    
    // Minimum track record length formula
    return std::pow(z_alpha * sharpe_std / sharpe, 2.0);
}

double SharpeRatioStats::calculate_expected_max_sharpe() const {
    // Simplified calculation - in practice would use more sophisticated methods
    double n = static_cast<double>(returns_.size());
    if (n <= 1.0) {
        return 0.0;
    }
    
    // Expected maximum Sharpe ratio based on sample size
    double c = std::sqrt(2.0 * std::log(n));
    return c;
}

double SharpeRatioStats::calculate_t_statistic_internal() const {
    if (returns_.empty()) {
        return 0.0;
    }
    
    double mean_return = calculate_mean();
    double std_return = calculate_std();
    double n = static_cast<double>(returns_.size());
    
    if (std_return == 0.0 || n <= 1.0) {
        return 0.0;
    }
    
    return (mean_return - p.risk_free_rate) * std::sqrt(n) / std_return;
}

double SharpeRatioStats::calculate_p_value_internal() const {
    double t_stat = calculate_t_statistic_internal();
    int df = static_cast<int>(returns_.size()) - 1;
    
    if (df <= 0) {
        return 1.0;
    }
    
    // Two-tailed test
    return 2.0 * (1.0 - student_t_cdf(std::abs(t_stat), df));
}

std::pair<double, double> SharpeRatioStats::calculate_confidence_interval() const {
    if (returns_.empty()) {
        return {0.0, 0.0};
    }
    
    double sharpe = calculate_sharpe_ratio();
    double sharpe_std = calculate_sharpe_std();
    
    // Confidence interval based on normal approximation
    double z_alpha_2 = 1.96; // 95% confidence level approximation
    double margin_of_error = z_alpha_2 * sharpe_std;
    
    return {sharpe - margin_of_error, sharpe + margin_of_error};
}

double SharpeRatioStats::calculate_mean() const {
    if (returns_.empty()) {
        return 0.0;
    }
    
    return std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
}

double SharpeRatioStats::calculate_std() const {
    if (returns_.size() <= 1) {
        return 0.0;
    }
    
    double mean = calculate_mean();
    double sq_sum = 0.0;
    
    for (double ret : returns_) {
        sq_sum += (ret - mean) * (ret - mean);
    }
    
    return std::sqrt(sq_sum / (returns_.size() - 1));
}

double SharpeRatioStats::calculate_skewness() const {
    if (returns_.size() <= 2) {
        return 0.0;
    }
    
    double mean = calculate_mean();
    double std_dev = calculate_std();
    
    if (std_dev == 0.0) {
        return 0.0;
    }
    
    double skew_sum = 0.0;
    for (double ret : returns_) {
        skew_sum += std::pow((ret - mean) / std_dev, 3.0);
    }
    
    return skew_sum / returns_.size();
}

double SharpeRatioStats::calculate_kurtosis() const {
    if (returns_.size() <= 3) {
        return 0.0;
    }
    
    double mean = calculate_mean();
    double std_dev = calculate_std();
    
    if (std_dev == 0.0) {
        return 0.0;
    }
    
    double kurt_sum = 0.0;
    for (double ret : returns_) {
        kurt_sum += std::pow((ret - mean) / std_dev, 4.0);
    }
    
    return (kurt_sum / returns_.size()) - 3.0; // Excess kurtosis
}

double SharpeRatioStats::calculate_var_95() const {
    if (returns_.empty()) {
        return 0.0;
    }
    
    auto sorted_returns = returns_;
    std::sort(sorted_returns.begin(), sorted_returns.end());
    
    size_t index = static_cast<size_t>(0.05 * sorted_returns.size());
    return -sorted_returns[index]; // VaR is positive
}

double SharpeRatioStats::calculate_cvar_95() const {
    if (returns_.empty()) {
        return 0.0;
    }
    
    auto sorted_returns = returns_;
    std::sort(sorted_returns.begin(), sorted_returns.end());
    
    size_t index = static_cast<size_t>(0.05 * sorted_returns.size());
    double sum = 0.0;
    
    for (size_t i = 0; i <= index && i < sorted_returns.size(); ++i) {
        sum += sorted_returns[i];
    }
    
    return -(sum / (index + 1)); // CVaR is positive
}

double SharpeRatioStats::calculate_maximum_drawdown() const {
    // Placeholder implementation
    return 0.0;
}

double SharpeRatioStats::calculate_information_ratio() const {
    // Placeholder implementation
    return 0.0;
}

double SharpeRatioStats::calculate_treynor_ratio() const {
    // Placeholder implementation
    return 0.0;
}

double SharpeRatioStats::calculate_sortino_ratio() const {
    // Placeholder implementation
    return 0.0;
}

double SharpeRatioStats::get_current_portfolio_value() const {
    // Placeholder implementation
    static double value = 100000.0;
    value += (rand() % 2001 - 1000) * 0.01;
    return value;
}

std::chrono::system_clock::time_point SharpeRatioStats::get_current_datetime() const {
    return std::chrono::system_clock::now();
}

double SharpeRatioStats::normal_cdf(double x) const {
    // Approximation of normal CDF
    return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
}

double SharpeRatioStats::normal_pdf(double x) const {
    return std::exp(-0.5 * x * x) / std::sqrt(2.0 * M_PI);
}

double SharpeRatioStats::student_t_cdf(double t, int df) const {
    // Simplified Student's t CDF approximation
    if (df > 30) {
        return normal_cdf(t);
    }
    
    // Very basic approximation - in practice would use more sophisticated methods
    return 0.5 + 0.5 * std::tanh(t / 2.0);
}

double SharpeRatioStats::gamma_function(double x) const {
    // Simplified gamma function approximation
    return std::exp(std::lgamma(x));
}

void SharpeRatioStats::invalidate_cache() {
    stats_cached_ = false;
}

void SharpeRatioStats::update_cache() const {
    if (stats_cached_) {
        return;
    }
    
    // Calculate all statistics
    cached_stats_.sharpe_ratio = calculate_sharpe_ratio();
    cached_stats_.annualized_sharpe = calculate_annualized_sharpe();
    cached_stats_.sharpe_std = calculate_sharpe_std();
    
    if (p.calculate_psr) {
        cached_stats_.probabilistic_sharpe_ratio = calculate_probabilistic_sharpe_ratio();
    }
    
    if (p.calculate_dsr) {
        cached_stats_.deflated_sharpe_ratio = calculate_deflated_sharpe_ratio();
    }
    
    if (p.calculate_min_trl) {
        cached_stats_.minimum_track_record_length = calculate_minimum_track_record_length();
    }
    
    cached_stats_.expected_max_sharpe = calculate_expected_max_sharpe();
    cached_stats_.t_statistic = calculate_t_statistic_internal();
    cached_stats_.p_value = calculate_p_value_internal();
    cached_stats_.is_significant = test_sharpe_significance();
    
    if (p.calculate_confidence_intervals) {
        auto ci = calculate_confidence_interval();
        cached_stats_.sharpe_lower_ci = ci.first;
        cached_stats_.sharpe_upper_ci = ci.second;
    }
    
    cached_stats_.sample_size = returns_.size();
    cached_stats_.mean_return = calculate_mean();
    cached_stats_.return_std = calculate_std();
    cached_stats_.skewness = calculate_skewness();
    cached_stats_.kurtosis = calculate_kurtosis();
    
    cached_stats_.var_95 = calculate_var_95();
    cached_stats_.cvar_95 = calculate_cvar_95();
    cached_stats_.maximum_drawdown = calculate_maximum_drawdown();
    
    cached_stats_.information_ratio = calculate_information_ratio();
    cached_stats_.treynor_ratio = calculate_treynor_ratio();
    cached_stats_.sortino_ratio = calculate_sortino_ratio();
    
    stats_cached_ = true;
}

void SharpeRatioStats::validate_parameters() const {
    if (p.confidence_level <= 0.0 || p.confidence_level >= 1.0) {
        throw std::invalid_argument("Confidence level must be between 0 and 1");
    }
    
    if (p.annualization_factor <= 0.0) {
        throw std::invalid_argument("Annualization factor must be positive");
    }
}

bool SharpeRatioStats::is_valid_return(double ret) const {
    return std::isfinite(ret) && !std::isnan(ret);
}

std::string SharpeRatioStats::format_datetime(const std::chrono::system_clock::time_point& dt) const {
    std::time_t time_t = std::chrono::system_clock::to_time_t(dt);
    std::tm* tm_ptr = std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    
    return oss.str();
}

std::string SharpeRatioStats::format_percentage(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << (value * 100.0) << "%";
    return oss.str();
}

} // namespace analyzers
} // namespace backtrader