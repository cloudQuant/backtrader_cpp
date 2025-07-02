#pragma once

#include "analyzers/AnalyzerBase.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace backtrader {

/**
 * @brief Portfolio returns analysis
 * 
 * Calculates comprehensive return statistics including:
 * - Total, annualized, and average returns
 * - Return distribution statistics
 * - Risk-adjusted metrics
 * - Time-based analysis
 */
class ReturnsAnalyzer : public AnalyzerBase {
public:
    /**
     * @brief Return statistics structure
     */
    struct ReturnStats {
        // Basic return metrics
        double total_return = 0.0;           // Total return over period
        double annualized_return = 0.0;      // Annualized return
        double average_return = 0.0;         // Average periodic return
        double geometric_return = 0.0;       // Geometric average return
        double compound_return = 0.0;        // Compound annual growth rate
        
        // Volatility metrics
        double volatility = 0.0;             // Annualized volatility
        double downside_volatility = 0.0;    // Downside volatility
        double upside_volatility = 0.0;      // Upside volatility
        double tracking_error = 0.0;         // Tracking error vs benchmark
        
        // Distribution statistics
        double skewness = 0.0;               // Return distribution skewness
        double kurtosis = 0.0;               // Return distribution kurtosis
        double min_return = 0.0;             // Minimum return
        double max_return = 0.0;             // Maximum return
        double median_return = 0.0;          // Median return
        
        // Percentiles
        double percentile_5 = 0.0;           // 5th percentile
        double percentile_25 = 0.0;          // 25th percentile
        double percentile_75 = 0.0;          // 75th percentile
        double percentile_95 = 0.0;          // 95th percentile
        
        // Period analysis
        int total_periods = 0;               // Total number of periods
        int positive_periods = 0;            // Number of positive return periods
        int negative_periods = 0;            // Number of negative return periods
        int zero_periods = 0;                // Number of zero return periods
        double win_rate = 0.0;               // Percentage of positive periods
        
        // Consecutive periods
        int max_consecutive_wins = 0;        // Maximum consecutive winning periods
        int max_consecutive_losses = 0;      // Maximum consecutive losing periods
        int current_streak = 0;              // Current streak (positive/negative)
        
        // Risk metrics
        double var_95 = 0.0;                 // 95% Value at Risk
        double cvar_95 = 0.0;                // 95% Conditional Value at Risk
        double maximum_loss = 0.0;           // Maximum single period loss
        double maximum_gain = 0.0;           // Maximum single period gain
        
        // Time-based returns
        double best_month = 0.0;             // Best monthly return
        double worst_month = 0.0;            // Worst monthly return
        double best_quarter = 0.0;           // Best quarterly return
        double worst_quarter = 0.0;          // Worst quarterly return
        double best_year = 0.0;              // Best annual return
        double worst_year = 0.0;             // Worst annual return
        
        // Rolling metrics
        double rolling_return_12m = 0.0;     // 12-month rolling return
        double rolling_volatility_12m = 0.0; // 12-month rolling volatility
        double rolling_sharpe_12m = 0.0;     // 12-month rolling Sharpe
        
        /**
         * @brief Check if returns are normally distributed (basic test)
         */
        bool isNormallyDistributed() const {
            return std::abs(skewness) < 0.5 && kurtosis < 3.0;
        }
        
        /**
         * @brief Get return-to-volatility ratio
         */
        double getReturnVolatilityRatio() const {
            return (volatility != 0.0) ? annualized_return / volatility : 0.0;
        }
        
        /**
         * @brief Get upside/downside volatility ratio
         */
        double getUpsideDownsideRatio() const {
            return (downside_volatility != 0.0) ? upside_volatility / downside_volatility : 0.0;
        }
        
        /**
         * @brief Reset all statistics
         */
        void reset() {
            *this = ReturnStats{};
        }
    };

private:
    ReturnStats stats_;
    std::vector<double> returns_;
    std::vector<double> portfolio_values_;
    std::vector<double> benchmark_returns_;
    
    double initial_value_;
    double risk_free_rate_;
    int periods_per_year_;
    
    // Streak tracking
    int current_streak_length_;
    bool current_streak_positive_;

public:
    explicit ReturnsAnalyzer(double risk_free_rate = 0.02,
                            int periods_per_year = 252,
                            const std::string& name = "ReturnsAnalyzer")
        : AnalyzerBase(name),
          initial_value_(0.0),
          risk_free_rate_(risk_free_rate),
          periods_per_year_(periods_per_year),
          current_streak_length_(0),
          current_streak_positive_(false) {}
    
    /**
     * @brief Initialize analyzer
     */
    void start() override {
        AnalyzerBase::start();
        stats_.reset();
        returns_.clear();
        portfolio_values_.clear();
        benchmark_returns_.clear();
        initial_value_ = 0.0;
        current_streak_length_ = 0;
        current_streak_positive_ = false;
    }
    
    /**
     * @brief Process new portfolio value
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
                
                // Update streak tracking
                updateStreaks(return_pct);
                
                // Update period counts
                stats_.total_periods++;
                if (return_pct > 0) {
                    stats_.positive_periods++;
                } else if (return_pct < 0) {
                    stats_.negative_periods++;
                } else {
                    stats_.zero_periods++;
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
    }
    
    /**
     * @brief Set periods per year for annualization
     */
    void setPeriodsPerYear(int periods) {
        periods_per_year_ = periods;
    }
    
    /**
     * @brief Get return statistics
     */
    const ReturnStats& getStats() const {
        return stats_;
    }
    
    /**
     * @brief Get returns series
     */
    const std::vector<double>& getReturns() const {
        return returns_;
    }
    
    /**
     * @brief Get analysis results as key-value pairs
     */
    std::map<std::string, double> getAnalysis() const override {
        std::map<std::string, double> result;
        
        result["total_return"] = stats_.total_return;
        result["annualized_return"] = stats_.annualized_return;
        result["average_return"] = stats_.average_return;
        result["geometric_return"] = stats_.geometric_return;
        result["compound_return"] = stats_.compound_return;
        
        result["volatility"] = stats_.volatility;
        result["downside_volatility"] = stats_.downside_volatility;
        result["upside_volatility"] = stats_.upside_volatility;
        result["tracking_error"] = stats_.tracking_error;
        
        result["skewness"] = stats_.skewness;
        result["kurtosis"] = stats_.kurtosis;
        result["min_return"] = stats_.min_return;
        result["max_return"] = stats_.max_return;
        result["median_return"] = stats_.median_return;
        
        result["percentile_5"] = stats_.percentile_5;
        result["percentile_25"] = stats_.percentile_25;
        result["percentile_75"] = stats_.percentile_75;
        result["percentile_95"] = stats_.percentile_95;
        
        result["total_periods"] = stats_.total_periods;
        result["positive_periods"] = stats_.positive_periods;
        result["negative_periods"] = stats_.negative_periods;
        result["win_rate"] = stats_.win_rate;
        
        result["max_consecutive_wins"] = stats_.max_consecutive_wins;
        result["max_consecutive_losses"] = stats_.max_consecutive_losses;
        result["current_streak"] = stats_.current_streak;
        
        result["var_95"] = stats_.var_95;
        result["cvar_95"] = stats_.cvar_95;
        result["maximum_loss"] = stats_.maximum_loss;
        result["maximum_gain"] = stats_.maximum_gain;
        
        return result;
    }
    
    /**
     * @brief Print analysis results
     */
    void printAnalysis() const override {
        std::cout << "=== Returns Analysis Results ===" << std::endl;
        std::cout << "Total Return: " << (stats_.total_return * 100) << "%" << std::endl;
        std::cout << "Annualized Return: " << (stats_.annualized_return * 100) << "%" << std::endl;
        std::cout << "Average Return: " << (stats_.average_return * 100) << "%" << std::endl;
        std::cout << "Geometric Return: " << (stats_.geometric_return * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Volatility: " << (stats_.volatility * 100) << "%" << std::endl;
        std::cout << "Downside Volatility: " << (stats_.downside_volatility * 100) << "%" << std::endl;
        std::cout << "Upside Volatility: " << (stats_.upside_volatility * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Distribution Statistics:" << std::endl;
        std::cout << "  Skewness: " << stats_.skewness << std::endl;
        std::cout << "  Kurtosis: " << stats_.kurtosis << std::endl;
        std::cout << "  Min Return: " << (stats_.min_return * 100) << "%" << std::endl;
        std::cout << "  Max Return: " << (stats_.max_return * 100) << "%" << std::endl;
        std::cout << "  Median Return: " << (stats_.median_return * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Period Analysis:" << std::endl;
        std::cout << "  Total Periods: " << stats_.total_periods << std::endl;
        std::cout << "  Win Rate: " << (stats_.win_rate * 100) << "%" << std::endl;
        std::cout << "  Max Consecutive Wins: " << stats_.max_consecutive_wins << std::endl;
        std::cout << "  Max Consecutive Losses: " << stats_.max_consecutive_losses << std::endl;
        std::cout << std::endl;
        
        std::cout << "Risk Metrics:" << std::endl;
        std::cout << "  VaR (95%): " << (stats_.var_95 * 100) << "%" << std::endl;
        std::cout << "  CVaR (95%): " << (stats_.cvar_95 * 100) << "%" << std::endl;
        std::cout << "  Maximum Loss: " << (stats_.maximum_loss * 100) << "%" << std::endl;
        std::cout << "  Maximum Gain: " << (stats_.maximum_gain * 100) << "%" << std::endl;
    }
    
    /**
     * @brief Get rolling returns for specified period
     */
    std::vector<double> getRollingReturns(size_t period) const {
        std::vector<double> rolling_returns;
        
        if (returns_.size() < period) {
            return rolling_returns;
        }
        
        for (size_t i = period - 1; i < returns_.size(); ++i) {
            double compound_return = 1.0;
            for (size_t j = i - period + 1; j <= i; ++j) {
                compound_return *= (1.0 + returns_[j]);
            }
            rolling_returns.push_back(compound_return - 1.0);
        }
        
        return rolling_returns;
    }

private:
    void calculateMetrics() {
        if (returns_.empty()) return;
        
        // Basic statistics
        stats_.average_return = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        stats_.win_rate = static_cast<double>(stats_.positive_periods) / stats_.total_periods;
        
        // Total and annualized returns
        if (portfolio_values_.size() >= 2 && initial_value_ != 0.0) {
            stats_.total_return = (portfolio_values_.back() - initial_value_) / initial_value_;
            
            // Annualized return
            double periods = static_cast<double>(returns_.size());
            stats_.annualized_return = std::pow(1.0 + stats_.total_return, 
                                              periods_per_year_ / periods) - 1.0;
            
            // Geometric return
            double product = 1.0;
            for (double ret : returns_) {
                product *= (1.0 + ret);
            }
            stats_.geometric_return = std::pow(product, 1.0 / returns_.size()) - 1.0;
            stats_.compound_return = std::pow(product, periods_per_year_ / periods) - 1.0;
        }
        
        // Volatility calculations
        calculateVolatilityMetrics();
        
        // Distribution statistics
        calculateDistributionStats();
        
        // Risk metrics
        calculateRiskMetrics();
        
        // Rolling metrics
        calculateRollingMetrics();
        
        // Benchmark comparison
        if (!benchmark_returns_.empty() && benchmark_returns_.size() == returns_.size()) {
            calculateTrackingError();
        }
    }
    
    void calculateVolatilityMetrics() {
        if (returns_.size() < 2) return;
        
        // Overall volatility
        double variance = 0.0;
        for (double ret : returns_) {
            variance += std::pow(ret - stats_.average_return, 2);
        }
        variance /= (returns_.size() - 1);
        stats_.volatility = std::sqrt(variance * periods_per_year_);
        
        // Downside and upside volatility
        double downside_variance = 0.0;
        double upside_variance = 0.0;
        int downside_count = 0;
        int upside_count = 0;
        
        double target = risk_free_rate_ / periods_per_year_;
        
        for (double ret : returns_) {
            if (ret < target) {
                downside_variance += std::pow(ret - target, 2);
                downside_count++;
            } else {
                upside_variance += std::pow(ret - target, 2);
                upside_count++;
            }
        }
        
        if (downside_count > 1) {
            downside_variance /= (downside_count - 1);
            stats_.downside_volatility = std::sqrt(downside_variance * periods_per_year_);
        }
        
        if (upside_count > 1) {
            upside_variance /= (upside_count - 1);
            stats_.upside_volatility = std::sqrt(upside_variance * periods_per_year_);
        }
    }
    
    void calculateDistributionStats() {
        if (returns_.empty()) return;
        
        // Min/Max
        auto minmax = std::minmax_element(returns_.begin(), returns_.end());
        stats_.min_return = *minmax.first;
        stats_.max_return = *minmax.second;
        stats_.maximum_loss = stats_.min_return;
        stats_.maximum_gain = stats_.max_return;
        
        // Median and percentiles
        std::vector<double> sorted_returns = returns_;
        std::sort(sorted_returns.begin(), sorted_returns.end());
        
        size_t n = sorted_returns.size();
        stats_.median_return = (n % 2 == 0) ? 
            (sorted_returns[n/2-1] + sorted_returns[n/2]) / 2.0 : 
            sorted_returns[n/2];
        
        stats_.percentile_5 = sorted_returns[static_cast<size_t>(n * 0.05)];
        stats_.percentile_25 = sorted_returns[static_cast<size_t>(n * 0.25)];
        stats_.percentile_75 = sorted_returns[static_cast<size_t>(n * 0.75)];
        stats_.percentile_95 = sorted_returns[static_cast<size_t>(n * 0.95)];
        
        // Skewness and kurtosis
        if (returns_.size() >= 3) {
            double mean = stats_.average_return;
            double std_dev = stats_.volatility / std::sqrt(periods_per_year_);
            
            double skew_sum = 0.0;
            double kurt_sum = 0.0;
            
            for (double ret : returns_) {
                double z = (ret - mean) / std_dev;
                skew_sum += z * z * z;
                kurt_sum += z * z * z * z;
            }
            
            stats_.skewness = skew_sum / returns_.size();
            stats_.kurtosis = kurt_sum / returns_.size() - 3.0;  // Excess kurtosis
        }
    }
    
    void calculateRiskMetrics() {
        if (returns_.size() < 20) return;
        
        std::vector<double> sorted_returns = returns_;
        std::sort(sorted_returns.begin(), sorted_returns.end());
        
        // VaR and CVaR
        size_t var_index = static_cast<size_t>(sorted_returns.size() * 0.05);
        if (var_index < sorted_returns.size()) {
            stats_.var_95 = sorted_returns[var_index];
            
            // CVaR (expected shortfall)
            double sum_tail = 0.0;
            for (size_t i = 0; i <= var_index; ++i) {
                sum_tail += sorted_returns[i];
            }
            stats_.cvar_95 = sum_tail / (var_index + 1);
        }
    }
    
    void calculateRollingMetrics() {
        // 12-month rolling metrics (if we have enough data)
        size_t months_12 = static_cast<size_t>(periods_per_year_);
        if (returns_.size() >= months_12) {
            auto start_12m = returns_.end() - months_12;
            std::vector<double> recent_12m(start_12m, returns_.end());
            
            // 12-month return
            double product_12m = 1.0;
            for (double ret : recent_12m) {
                product_12m *= (1.0 + ret);
            }
            stats_.rolling_return_12m = product_12m - 1.0;
            
            // 12-month volatility
            double mean_12m = std::accumulate(recent_12m.begin(), recent_12m.end(), 0.0) / months_12;
            double var_12m = 0.0;
            for (double ret : recent_12m) {
                var_12m += std::pow(ret - mean_12m, 2);
            }
            var_12m /= (months_12 - 1);
            stats_.rolling_volatility_12m = std::sqrt(var_12m * periods_per_year_);
            
            // 12-month Sharpe
            double excess_return_12m = stats_.rolling_return_12m - risk_free_rate_;
            stats_.rolling_sharpe_12m = (stats_.rolling_volatility_12m != 0.0) ? 
                excess_return_12m / stats_.rolling_volatility_12m : 0.0;
        }
    }
    
    void calculateTrackingError() {
        std::vector<double> tracking_errors;
        for (size_t i = 0; i < std::min(returns_.size(), benchmark_returns_.size()); ++i) {
            tracking_errors.push_back(returns_[i] - benchmark_returns_[i]);
        }
        
        if (!tracking_errors.empty()) {
            double te_mean = std::accumulate(tracking_errors.begin(), tracking_errors.end(), 0.0) / tracking_errors.size();
            double te_variance = 0.0;
            for (double te : tracking_errors) {
                te_variance += std::pow(te - te_mean, 2);
            }
            te_variance /= (tracking_errors.size() - 1);
            stats_.tracking_error = std::sqrt(te_variance * periods_per_year_);
        }
    }
    
    void updateStreaks(double return_pct) {
        bool is_positive = return_pct > 0;
        
        if (current_streak_length_ == 0) {
            // Starting first streak
            current_streak_positive_ = is_positive;
            current_streak_length_ = 1;
        } else if (current_streak_positive_ == is_positive) {
            // Continuing current streak
            current_streak_length_++;
        } else {
            // Streak ended, update maximums
            if (current_streak_positive_) {
                stats_.max_consecutive_wins = std::max(stats_.max_consecutive_wins, current_streak_length_);
            } else {
                stats_.max_consecutive_losses = std::max(stats_.max_consecutive_losses, current_streak_length_);
            }
            
            // Start new streak
            current_streak_positive_ = is_positive;
            current_streak_length_ = 1;
        }
        
        // Update current streak (positive for wins, negative for losses)
        stats_.current_streak = current_streak_positive_ ? current_streak_length_ : -current_streak_length_;
    }
};

} // namespace backtrader