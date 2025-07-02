#pragma once

#include "analyzers/AnalyzerBase.h"
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace backtrader {

/**
 * @brief Sharpe Ratio analysis for risk-adjusted returns
 * 
 * Calculates various forms of Sharpe ratio including:
 * - Classic Sharpe Ratio
 * - Modified Sharpe Ratio
 * - Sortino Ratio
 * - Calmar Ratio
 * - Information Ratio
 */
class SharpeRatioAnalyzer : public AnalyzerBase {
public:
    /**
     * @brief Sharpe ratio statistics structure
     */
    struct SharpeStats {
        // Basic metrics
        double total_return = 0.0;                // Total return
        double annualized_return = 0.0;           // Annualized return
        double volatility = 0.0;                  // Annualized volatility (std dev)
        double downside_volatility = 0.0;         // Downside volatility
        double risk_free_rate = 0.0;              // Risk-free rate
        
        // Sharpe ratios
        double sharpe_ratio = 0.0;                // Classic Sharpe ratio
        double annualized_sharpe = 0.0;           // Annualized Sharpe ratio
        double modified_sharpe = 0.0;             // Modified Sharpe (using VaR)
        double sortino_ratio = 0.0;               // Sortino ratio (downside risk)
        double calmar_ratio = 0.0;                // Calmar ratio (return/max drawdown)
        double information_ratio = 0.0;           // Information ratio vs benchmark
        
        // Return statistics
        double average_return = 0.0;              // Average period return
        double excess_return = 0.0;               // Average excess return
        double skewness = 0.0;                    // Return distribution skewness
        double kurtosis = 0.0;                    // Return distribution kurtosis
        
        // Risk metrics
        double max_drawdown = 0.0;                // Maximum drawdown
        double var_95 = 0.0;                      // 95% Value at Risk
        double cvar_95 = 0.0;                     // 95% Conditional VaR
        double beta = 0.0;                        // Beta vs benchmark
        double alpha = 0.0;                       // Alpha vs benchmark
        double treynor_ratio = 0.0;               // Treynor ratio
        
        // Period statistics
        int total_periods = 0;                    // Total number of periods
        int positive_periods = 0;                 // Number of positive return periods
        int negative_periods = 0;                 // Number of negative return periods
        double win_rate = 0.0;                    // Percentage of positive periods
        
        // Rolling metrics
        double rolling_sharpe_30 = 0.0;           // 30-period rolling Sharpe
        double rolling_sharpe_90 = 0.0;           // 90-period rolling Sharpe
        double rolling_volatility_30 = 0.0;       // 30-period rolling volatility
        
        /**
         * @brief Check if performance is acceptable
         */
        bool isAcceptablePerformance(double min_sharpe = 1.0) const {
            return sharpe_ratio >= min_sharpe;
        }
        
        /**
         * @brief Get risk-adjusted performance ranking
         */
        std::string getPerformanceRating() const {
            if (sharpe_ratio >= 3.0) return "Excellent";
            else if (sharpe_ratio >= 2.0) return "Very Good";
            else if (sharpe_ratio >= 1.0) return "Good";
            else if (sharpe_ratio >= 0.5) return "Acceptable";
            else if (sharpe_ratio >= 0.0) return "Poor";
            else return "Very Poor";
        }
        
        /**
         * @brief Reset all statistics
         */
        void reset() {
            *this = SharpeStats{};
        }
    };

private:
    SharpeStats stats_;
    std::vector<double> returns_;
    std::vector<double> benchmark_returns_;
    std::vector<double> portfolio_values_;
    
    double risk_free_rate_;
    int periods_per_year_;
    bool use_excess_returns_;
    double initial_value_;

public:
    explicit SharpeRatioAnalyzer(double risk_free_rate = 0.02,
                                int periods_per_year = 252,
                                bool use_excess_returns = true,
                                const std::string& name = "SharpeRatioAnalyzer")
        : AnalyzerBase(name),
          risk_free_rate_(risk_free_rate),
          periods_per_year_(periods_per_year),
          use_excess_returns_(use_excess_returns),
          initial_value_(0.0) {}
    
    /**
     * @brief Initialize analyzer
     */
    void start() override {
        AnalyzerBase::start();
        stats_.reset();
        returns_.clear();
        benchmark_returns_.clear();
        portfolio_values_.clear();
        stats_.risk_free_rate = risk_free_rate_;
        initial_value_ = 0.0;
    }
    
    /**
     * @brief Process new portfolio value
     * @param portfolio_value Current portfolio value
     * @param benchmark_value Optional benchmark value
     */
    void updatePortfolioValue(double portfolio_value, double benchmark_value = NaN) {
        portfolio_values_.push_back(portfolio_value);
        
        if (initial_value_ == 0.0) {
            initial_value_ = portfolio_value;
            return;
        }
        
        // Calculate return
        if (portfolio_values_.size() > 1) {
            double prev_value = portfolio_values_[portfolio_values_.size() - 2];
            if (prev_value != 0.0) {
                double return_pct = (portfolio_value - prev_value) / prev_value;
                returns_.push_back(return_pct);
                
                // Update period statistics
                stats_.total_periods++;
                if (return_pct > 0) {
                    stats_.positive_periods++;
                } else if (return_pct < 0) {
                    stats_.negative_periods++;
                }
            }
        }
        
        // Process benchmark return if provided
        if (!isNaN(benchmark_value)) {
            static double prev_benchmark = NaN;
            if (!isNaN(prev_benchmark) && prev_benchmark != 0.0) {
                double benchmark_return = (benchmark_value - prev_benchmark) / prev_benchmark;
                benchmark_returns_.push_back(benchmark_return);
            }
            prev_benchmark = benchmark_value;
        }
        
        // Calculate metrics if we have enough data
        if (returns_.size() >= 2) {
            calculateMetrics();
        }
    }
    
    /**
     * @brief Set risk-free rate
     */
    void setRiskFreeRate(double rate) {
        risk_free_rate_ = rate;
        stats_.risk_free_rate = rate;
    }
    
    /**
     * @brief Set periods per year for annualization
     */
    void setPeriodsPerYear(int periods) {
        periods_per_year_ = periods;
    }
    
    /**
     * @brief Set maximum drawdown for Calmar ratio
     */
    void setMaxDrawdown(double max_drawdown) {
        stats_.max_drawdown = max_drawdown;
        if (max_drawdown != 0.0) {
            stats_.calmar_ratio = stats_.annualized_return / max_drawdown;
        }
    }
    
    /**
     * @brief Get Sharpe statistics
     */
    const SharpeStats& getStats() const {
        return stats_;
    }
    
    /**
     * @brief Get analysis results as key-value pairs
     */
    std::map<std::string, double> getAnalysis() const override {
        std::map<std::string, double> result;
        
        result["total_return"] = stats_.total_return;
        result["annualized_return"] = stats_.annualized_return;
        result["volatility"] = stats_.volatility;
        result["downside_volatility"] = stats_.downside_volatility;
        result["risk_free_rate"] = stats_.risk_free_rate;
        
        result["sharpe_ratio"] = stats_.sharpe_ratio;
        result["annualized_sharpe"] = stats_.annualized_sharpe;
        result["modified_sharpe"] = stats_.modified_sharpe;
        result["sortino_ratio"] = stats_.sortino_ratio;
        result["calmar_ratio"] = stats_.calmar_ratio;
        result["information_ratio"] = stats_.information_ratio;
        
        result["average_return"] = stats_.average_return;
        result["excess_return"] = stats_.excess_return;
        result["skewness"] = stats_.skewness;
        result["kurtosis"] = stats_.kurtosis;
        
        result["max_drawdown"] = stats_.max_drawdown;
        result["var_95"] = stats_.var_95;
        result["cvar_95"] = stats_.cvar_95;
        result["beta"] = stats_.beta;
        result["alpha"] = stats_.alpha;
        result["treynor_ratio"] = stats_.treynor_ratio;
        
        result["total_periods"] = stats_.total_periods;
        result["positive_periods"] = stats_.positive_periods;
        result["negative_periods"] = stats_.negative_periods;
        result["win_rate"] = stats_.win_rate;
        
        result["rolling_sharpe_30"] = stats_.rolling_sharpe_30;
        result["rolling_sharpe_90"] = stats_.rolling_sharpe_90;
        result["rolling_volatility_30"] = stats_.rolling_volatility_30;
        
        return result;
    }
    
    /**
     * @brief Print analysis results
     */
    void printAnalysis() const override {
        std::cout << "=== Sharpe Ratio Analysis Results ===" << std::endl;
        std::cout << "Performance Rating: " << stats_.getPerformanceRating() << std::endl;
        std::cout << std::endl;
        
        std::cout << "Returns:" << std::endl;
        std::cout << "  Total Return: " << (stats_.total_return * 100) << "%" << std::endl;
        std::cout << "  Annualized Return: " << (stats_.annualized_return * 100) << "%" << std::endl;
        std::cout << "  Average Period Return: " << (stats_.average_return * 100) << "%" << std::endl;
        std::cout << "  Excess Return: " << (stats_.excess_return * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Risk Metrics:" << std::endl;
        std::cout << "  Volatility: " << (stats_.volatility * 100) << "%" << std::endl;
        std::cout << "  Downside Volatility: " << (stats_.downside_volatility * 100) << "%" << std::endl;
        std::cout << "  VaR (95%): " << (stats_.var_95 * 100) << "%" << std::endl;
        std::cout << "  CVaR (95%): " << (stats_.cvar_95 * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Risk-Adjusted Returns:" << std::endl;
        std::cout << "  Sharpe Ratio: " << stats_.sharpe_ratio << std::endl;
        std::cout << "  Annualized Sharpe: " << stats_.annualized_sharpe << std::endl;
        std::cout << "  Sortino Ratio: " << stats_.sortino_ratio << std::endl;
        std::cout << "  Calmar Ratio: " << stats_.calmar_ratio << std::endl;
        std::cout << "  Information Ratio: " << stats_.information_ratio << std::endl;
        std::cout << "  Treynor Ratio: " << stats_.treynor_ratio << std::endl;
        std::cout << std::endl;
        
        std::cout << "Distribution Statistics:" << std::endl;
        std::cout << "  Skewness: " << stats_.skewness << std::endl;
        std::cout << "  Kurtosis: " << stats_.kurtosis << std::endl;
        std::cout << "  Win Rate: " << (stats_.win_rate * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        if (!benchmark_returns_.empty()) {
            std::cout << "Benchmark Comparison:" << std::endl;
            std::cout << "  Beta: " << stats_.beta << std::endl;
            std::cout << "  Alpha: " << (stats_.alpha * 100) << "%" << std::endl;
            std::cout << std::endl;
        }
        
        std::cout << "Rolling Metrics:" << std::endl;
        std::cout << "  30-Period Rolling Sharpe: " << stats_.rolling_sharpe_30 << std::endl;
        std::cout << "  90-Period Rolling Sharpe: " << stats_.rolling_sharpe_90 << std::endl;
        std::cout << "  30-Period Rolling Volatility: " << (stats_.rolling_volatility_30 * 100) << "%" << std::endl;
    }

private:
    void calculateMetrics() {
        if (returns_.empty()) return;
        
        // Basic statistics
        stats_.average_return = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        stats_.win_rate = static_cast<double>(stats_.positive_periods) / stats_.total_periods;
        
        // Risk-free rate per period
        double rf_period = risk_free_rate_ / periods_per_year_;
        stats_.excess_return = stats_.average_return - rf_period;
        
        // Volatility calculation
        double variance = 0.0;
        for (double ret : returns_) {
            variance += std::pow(ret - stats_.average_return, 2);
        }
        variance /= (returns_.size() - 1);
        stats_.volatility = std::sqrt(variance * periods_per_year_);
        
        // Downside volatility (Sortino denominator)
        double downside_variance = 0.0;
        int downside_count = 0;
        for (double ret : returns_) {
            if (ret < rf_period) {
                downside_variance += std::pow(ret - rf_period, 2);
                downside_count++;
            }
        }
        if (downside_count > 1) {
            downside_variance /= (downside_count - 1);
            stats_.downside_volatility = std::sqrt(downside_variance * periods_per_year_);
        }
        
        // Total and annualized returns
        if (portfolio_values_.size() >= 2 && initial_value_ != 0.0) {
            stats_.total_return = (portfolio_values_.back() - initial_value_) / initial_value_;
            double periods = static_cast<double>(returns_.size());
            stats_.annualized_return = std::pow(1.0 + stats_.total_return, 
                                              periods_per_year_ / periods) - 1.0;
        }
        
        // Sharpe ratios
        if (stats_.volatility != 0.0) {
            stats_.sharpe_ratio = stats_.excess_return / (stats_.volatility / std::sqrt(periods_per_year_));
            stats_.annualized_sharpe = (stats_.annualized_return - risk_free_rate_) / stats_.volatility;
        }
        
        // Sortino ratio
        if (stats_.downside_volatility != 0.0) {
            stats_.sortino_ratio = (stats_.annualized_return - risk_free_rate_) / stats_.downside_volatility;
        }
        
        // VaR and CVaR
        calculateVaRMetrics();
        
        // Modified Sharpe using VaR
        if (stats_.var_95 != 0.0) {
            stats_.modified_sharpe = stats_.excess_return / std::abs(stats_.var_95);
        }
        
        // Distribution moments
        calculateDistributionMoments();
        
        // Benchmark-related metrics
        if (!benchmark_returns_.empty() && benchmark_returns_.size() == returns_.size()) {
            calculateBenchmarkMetrics();
        }
        
        // Rolling metrics
        calculateRollingMetrics();
    }
    
    void calculateVaRMetrics() {
        if (returns_.size() < 20) return;
        
        std::vector<double> sorted_returns = returns_;
        std::sort(sorted_returns.begin(), sorted_returns.end());
        
        // 95% VaR
        size_t var_index = static_cast<size_t>(sorted_returns.size() * 0.05);
        if (var_index < sorted_returns.size()) {
            stats_.var_95 = sorted_returns[var_index];
            
            // CVaR
            double sum_tail = 0.0;
            for (size_t i = 0; i <= var_index; ++i) {
                sum_tail += sorted_returns[i];
            }
            stats_.cvar_95 = sum_tail / (var_index + 1);
        }
    }
    
    void calculateDistributionMoments() {
        if (returns_.size() < 4) return;
        
        double mean = stats_.average_return;
        double variance = 0.0;
        double skew_sum = 0.0;
        double kurt_sum = 0.0;
        
        for (double ret : returns_) {
            double diff = ret - mean;
            double diff2 = diff * diff;
            variance += diff2;
            skew_sum += diff * diff2;
            kurt_sum += diff2 * diff2;
        }
        
        size_t n = returns_.size();
        variance /= (n - 1);
        double std_dev = std::sqrt(variance);
        
        if (std_dev != 0.0) {
            stats_.skewness = (skew_sum / n) / std::pow(std_dev, 3);
            stats_.kurtosis = (kurt_sum / n) / std::pow(std_dev, 4) - 3.0;  // Excess kurtosis
        }
    }
    
    void calculateBenchmarkMetrics() {
        // Calculate beta and alpha using linear regression
        double sum_xy = 0.0, sum_x = 0.0, sum_y = 0.0, sum_x2 = 0.0;
        size_t n = std::min(returns_.size(), benchmark_returns_.size());
        
        for (size_t i = 0; i < n; ++i) {
            double x = benchmark_returns_[i];
            double y = returns_[i];
            sum_xy += x * y;
            sum_x += x;
            sum_y += y;
            sum_x2 += x * x;
        }
        
        double mean_x = sum_x / n;
        double mean_y = sum_y / n;
        
        double numerator = sum_xy - n * mean_x * mean_y;
        double denominator = sum_x2 - n * mean_x * mean_x;
        
        if (denominator != 0.0) {
            stats_.beta = numerator / denominator;
            stats_.alpha = mean_y - stats_.beta * mean_x;
            
            // Treynor ratio
            if (stats_.beta != 0.0) {
                stats_.treynor_ratio = (stats_.annualized_return - risk_free_rate_) / stats_.beta;
            }
            
            // Information ratio (tracking error)
            std::vector<double> tracking_errors;
            for (size_t i = 0; i < n; ++i) {
                tracking_errors.push_back(returns_[i] - benchmark_returns_[i]);
            }
            
            if (!tracking_errors.empty()) {
                double te_mean = std::accumulate(tracking_errors.begin(), tracking_errors.end(), 0.0) / tracking_errors.size();
                double te_variance = 0.0;
                for (double te : tracking_errors) {
                    te_variance += std::pow(te - te_mean, 2);
                }
                te_variance /= (tracking_errors.size() - 1);
                double tracking_error = std::sqrt(te_variance * periods_per_year_);
                
                if (tracking_error != 0.0) {
                    stats_.information_ratio = te_mean * std::sqrt(periods_per_year_) / tracking_error;
                }
            }
        }
    }
    
    void calculateRollingMetrics() {
        // 30-period rolling Sharpe
        if (returns_.size() >= 30) {
            auto start_30 = returns_.end() - 30;
            std::vector<double> recent_30(start_30, returns_.end());
            stats_.rolling_sharpe_30 = calculateSharpeForPeriod(recent_30);
            
            // Rolling volatility
            double mean_30 = std::accumulate(recent_30.begin(), recent_30.end(), 0.0) / 30;
            double var_30 = 0.0;
            for (double ret : recent_30) {
                var_30 += std::pow(ret - mean_30, 2);
            }
            var_30 /= 29;
            stats_.rolling_volatility_30 = std::sqrt(var_30 * periods_per_year_);
        }
        
        // 90-period rolling Sharpe
        if (returns_.size() >= 90) {
            auto start_90 = returns_.end() - 90;
            std::vector<double> recent_90(start_90, returns_.end());
            stats_.rolling_sharpe_90 = calculateSharpeForPeriod(recent_90);
        }
    }
    
    double calculateSharpeForPeriod(const std::vector<double>& period_returns) {
        if (period_returns.empty()) return 0.0;
        
        double mean_return = std::accumulate(period_returns.begin(), period_returns.end(), 0.0) / period_returns.size();
        double rf_period = risk_free_rate_ / periods_per_year_;
        double excess = mean_return - rf_period;
        
        double variance = 0.0;
        for (double ret : period_returns) {
            variance += std::pow(ret - mean_return, 2);
        }
        variance /= (period_returns.size() - 1);
        double volatility = std::sqrt(variance);
        
        return (volatility != 0.0) ? excess / volatility * std::sqrt(periods_per_year_) : 0.0;
    }
};

} // namespace backtrader