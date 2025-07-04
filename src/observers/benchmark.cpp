#include "../../include/observers/benchmark.h"
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace observers {

Benchmark::Benchmark(const Params& params) 
    : p(params), initial_value_(0.0), initial_benchmark_(0.0), 
      benchmark_initialized_(false) {
    
    // Initialize lines
    lines.resize(3);
    lines[0].clear(); // benchmark returns
    lines[1].clear(); // strategy returns
    lines[2].clear(); // relative performance
    
    // Set line names
    line_names.resize(3);
    line_names[0] = "benchmark";
    line_names[1] = "strategy";
    line_names[2] = "relative";
}

void Benchmark::prenext() {
    // Called before next() during minimum period
    next();
}

void Benchmark::next() {
    if (!broker_) {
        return;
    }
    
    // Get current strategy value
    double current_value = broker_->get_value();
    
    // Get benchmark data
    double benchmark_price = 0.0;
    
    if (p.data_benchmark) {
        // Use specified benchmark data
        if (!p.data_benchmark->lines.empty() && !p.data_benchmark->lines[4].empty()) {
            benchmark_price = p.data_benchmark->lines[4].back(); // Close price
        }
    } else if (data_) {
        // Use main data as benchmark
        if (!data_->lines.empty() && !data_->lines[4].empty()) {
            benchmark_price = data_->lines[4].back(); // Close price
        }
    }
    
    // Initialize on first call
    if (!benchmark_initialized_ && benchmark_price > 0.0) {
        initial_value_ = current_value;
        initial_benchmark_ = benchmark_price;
        benchmark_initialized_ = true;
    }
    
    if (benchmark_initialized_) {
        // Calculate returns
        double strategy_return = 0.0;
        double benchmark_return = 0.0;
        
        if (initial_value_ > 0.0) {
            strategy_return = (current_value - initial_value_) / initial_value_;
        }
        
        if (initial_benchmark_ > 0.0) {
            benchmark_return = (benchmark_price - initial_benchmark_) / initial_benchmark_;
        }
        
        // Calculate relative performance
        double relative_perf = strategy_return - benchmark_return;
        
        // Store in lines (as percentages)
        lines[0].push_back(benchmark_return * 100.0);
        lines[1].push_back(strategy_return * 100.0);
        lines[2].push_back(relative_perf * 100.0);
    }
}

double Benchmark::get_strategy_return() const {
    if (!lines[1].empty()) {
        return lines[1].back() / 100.0; // Convert from percentage
    }
    return 0.0;
}

double Benchmark::get_benchmark_return() const {
    if (!lines[0].empty()) {
        return lines[0].back() / 100.0; // Convert from percentage
    }
    return 0.0;
}

double Benchmark::get_relative_return() const {
    if (!lines[2].empty()) {
        return lines[2].back() / 100.0; // Convert from percentage
    }
    return 0.0;
}

std::map<std::string, double> Benchmark::get_analysis() const {
    std::map<std::string, double> analysis;
    
    // Current returns
    analysis["strategy_return"] = get_strategy_return();
    analysis["benchmark_return"] = get_benchmark_return();
    analysis["relative_return"] = get_relative_return();
    analysis["alpha"] = get_relative_return(); // Simplified alpha
    
    if (!lines[0].empty() && !lines[1].empty()) {
        const auto& benchmark_returns = lines[0];
        const auto& strategy_returns = lines[1];
        
        // Calculate total returns
        double total_strategy_return = strategy_returns.back() / 100.0;
        double total_benchmark_return = benchmark_returns.back() / 100.0;
        
        analysis["total_strategy_return"] = total_strategy_return;
        analysis["total_benchmark_return"] = total_benchmark_return;
        analysis["total_relative_return"] = total_strategy_return - total_benchmark_return;
        
        // Calculate beta (simplified)
        double beta = calculate_beta(strategy_returns, benchmark_returns);
        analysis["beta"] = beta;
        
        // Calculate correlation
        double correlation = calculate_correlation(strategy_returns, benchmark_returns);
        analysis["correlation"] = correlation;
        
        // Calculate tracking error
        double tracking_error = calculate_tracking_error(strategy_returns, benchmark_returns);
        analysis["tracking_error"] = tracking_error;
        
        // Calculate information ratio
        if (tracking_error > 0.0) {
            analysis["information_ratio"] = (total_strategy_return - total_benchmark_return) / tracking_error;
        } else {
            analysis["information_ratio"] = 0.0;
        }
        
        // Win/loss periods against benchmark
        int periods_outperformed = 0;
        int periods_underperformed = 0;
        
        for (size_t i = 0; i < lines[2].size(); ++i) {
            if (lines[2][i] > 0.0) {
                periods_outperformed++;
            } else if (lines[2][i] < 0.0) {
                periods_underperformed++;
            }
        }
        
        analysis["periods_outperformed"] = static_cast<double>(periods_outperformed);
        analysis["periods_underperformed"] = static_cast<double>(periods_underperformed);
        
        if (lines[2].size() > 0) {
            analysis["win_rate_vs_benchmark"] = static_cast<double>(periods_outperformed) / lines[2].size();
        }
    }
    
    return analysis;
}

double Benchmark::calculate_beta(const std::vector<double>& strategy_returns, 
                                const std::vector<double>& benchmark_returns) const {
    if (strategy_returns.size() < 2 || benchmark_returns.size() < 2) {
        return 1.0; // Default beta
    }
    
    // Calculate daily returns (differences)
    std::vector<double> strategy_daily, benchmark_daily;
    
    for (size_t i = 1; i < strategy_returns.size(); ++i) {
        strategy_daily.push_back(strategy_returns[i] - strategy_returns[i-1]);
    }
    
    for (size_t i = 1; i < benchmark_returns.size(); ++i) {
        benchmark_daily.push_back(benchmark_returns[i] - benchmark_returns[i-1]);
    }
    
    // Calculate covariance and variance
    double covariance = calculate_covariance(strategy_daily, benchmark_daily);
    double benchmark_variance = calculate_variance(benchmark_daily);
    
    if (benchmark_variance > 0.0) {
        return covariance / benchmark_variance;
    }
    
    return 1.0;
}

double Benchmark::calculate_correlation(const std::vector<double>& x, 
                                      const std::vector<double>& y) const {
    if (x.size() != y.size() || x.size() < 2) {
        return 0.0;
    }
    
    double covariance = calculate_covariance(x, y);
    double std_x = std::sqrt(calculate_variance(x));
    double std_y = std::sqrt(calculate_variance(y));
    
    if (std_x > 0.0 && std_y > 0.0) {
        return covariance / (std_x * std_y);
    }
    
    return 0.0;
}

double Benchmark::calculate_covariance(const std::vector<double>& x, 
                                     const std::vector<double>& y) const {
    if (x.size() != y.size() || x.empty()) {
        return 0.0;
    }
    
    double mean_x = calculate_mean(x);
    double mean_y = calculate_mean(y);
    
    double sum = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        sum += (x[i] - mean_x) * (y[i] - mean_y);
    }
    
    return sum / x.size();
}

double Benchmark::calculate_variance(const std::vector<double>& data) const {
    if (data.empty()) {
        return 0.0;
    }
    
    double mean = calculate_mean(data);
    double sum = 0.0;
    
    for (double value : data) {
        double diff = value - mean;
        sum += diff * diff;
    }
    
    return sum / data.size();
}

double Benchmark::calculate_mean(const std::vector<double>& data) const {
    if (data.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double value : data) {
        sum += value;
    }
    
    return sum / data.size();
}

double Benchmark::calculate_tracking_error(const std::vector<double>& strategy_returns,
                                         const std::vector<double>& benchmark_returns) const {
    if (strategy_returns.size() != benchmark_returns.size() || strategy_returns.empty()) {
        return 0.0;
    }
    
    // Calculate differences
    std::vector<double> differences;
    for (size_t i = 0; i < strategy_returns.size(); ++i) {
        differences.push_back(strategy_returns[i] - benchmark_returns[i]);
    }
    
    // Return standard deviation of differences
    return std::sqrt(calculate_variance(differences));
}

void Benchmark::reset() {
    initial_value_ = 0.0;
    initial_benchmark_ = 0.0;
    benchmark_initialized_ = false;
    
    for (auto& line : lines) {
        line.clear();
    }
}

void Benchmark::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

void Benchmark::set_data(std::shared_ptr<DataSeries> data) {
    data_ = data;
}

std::vector<double> Benchmark::get_cumulative_returns() const {
    std::vector<double> cumulative;
    
    if (!lines[1].empty()) {
        cumulative.reserve(lines[1].size());
        
        double cum_return = 1.0;
        for (double ret : lines[1]) {
            cum_return *= (1.0 + ret / 100.0);
            cumulative.push_back((cum_return - 1.0) * 100.0);
        }
    }
    
    return cumulative;
}

std::vector<double> Benchmark::get_benchmark_cumulative_returns() const {
    std::vector<double> cumulative;
    
    if (!lines[0].empty()) {
        cumulative.reserve(lines[0].size());
        
        double cum_return = 1.0;
        for (double ret : lines[0]) {
            cum_return *= (1.0 + ret / 100.0);
            cumulative.push_back((cum_return - 1.0) * 100.0);
        }
    }
    
    return cumulative;
}

} // namespace observers
} // namespace backtrader