#pragma once

#include "analyzers/AnalyzerBase.h"
#include <vector>
#include <cmath>

namespace backtrader {

/**
 * @brief Sharpe Ratio analyzer for risk-adjusted return analysis
 * 
 * The Sharpe Ratio measures the performance of an investment compared to a risk-free asset,
 * after adjusting for its risk. It's calculated as:
 * 
 * Sharpe Ratio = (Portfolio Return - Risk Free Rate) / Portfolio Volatility
 * 
 * Higher Sharpe ratios indicate better risk-adjusted performance.
 * - > 1.0: Good risk-adjusted returns
 * - > 2.0: Very good risk-adjusted returns
 * - > 3.0: Excellent risk-adjusted returns
 */
class SharpeRatio : public AnalyzerBase {
public:
    /**
     * @brief Sharpe ratio statistics structure
     */
    struct SharpeStats {
        // Basic Sharpe ratio metrics
        double sharpe_ratio = 0.0;              // Annualized Sharpe ratio
        double sharpe_ratio_daily = 0.0;        // Daily Sharpe ratio
        double sharpe_ratio_monthly = 0.0;      // Monthly Sharpe ratio
        
        // Input metrics
        double portfolio_return = 0.0;          // Annualized portfolio return
        double portfolio_volatility = 0.0;      // Annualized portfolio volatility
        double risk_free_rate = 0.0;           // Risk-free rate used
        double excess_return = 0.0;             // Portfolio return - risk-free rate
        
        // Return statistics
        double average_return = 0.0;            // Average period return
        double return_std = 0.0;                // Standard deviation of returns
        double geometric_return = 0.0;          // Geometric average return
        
        // Risk metrics
        double downside_volatility = 0.0;       // Volatility of negative returns
        double upside_volatility = 0.0;         // Volatility of positive returns
        double sortino_ratio = 0.0;             // Sortino ratio (excess return / downside vol)
        double calmar_ratio = 0.0;              // Calmar ratio (annual return / max drawdown)
        
        // Additional ratios
        double treynor_ratio = 0.0;             // Treynor ratio (if beta available)
        double information_ratio = 0.0;         // Information ratio vs benchmark
        double appraisal_ratio = 0.0;           // Jensen's alpha / tracking error
        
        // Time-based metrics
        int total_periods = 0;                  // Total number of periods analyzed
        int periods_per_year = 252;             // Periods per year for annualization
        double time_period_years = 0.0;         // Total time period in years
        
        // Distribution metrics
        double skewness = 0.0;                  // Return distribution skewness
        double kurtosis = 0.0;                  // Return distribution excess kurtosis
        double var_95 = 0.0;                    // 95% Value at Risk
        double cvar_95 = 0.0;                   // 95% Conditional Value at Risk
        
        /**
         * @brief Get Sharpe ratio quality assessment
         */
        std::string getQualityAssessment() const {
            if (sharpe_ratio > 3.0) return "Excellent";
            else if (sharpe_ratio > 2.0) return "Very Good";
            else if (sharpe_ratio > 1.0) return "Good";
            else if (sharpe_ratio > 0.5) return "Acceptable";
            else if (sharpe_ratio > 0.0) return "Poor";
            else return "Negative";
        }
        
        /**
         * @brief Get risk-return efficiency
         */
        double getRiskReturnEfficiency() const {
            return (portfolio_volatility != 0.0) ? portfolio_return / portfolio_volatility : 0.0;
        }
        
        /**
         * @brief Check if returns are normally distributed
         */
        bool isNormallyDistributed() const {
            return std::abs(skewness) < 0.5 && kurtosis < 3.0;
        }
        
        /**
         * @brief Reset all statistics
         */
        void reset() {
            *this = SharpeStats{};
            periods_per_year = 252;  // Reset default
        }
    };

private:
    SharpeStats stats_;
    std::vector<double> returns_;
    std::vector<double> portfolio_values_;
    std::vector<double> benchmark_returns_;
    
    double risk_free_rate_;
    int periods_per_year_;
    double initial_value_;
    bool use_log_returns_;
    
public:
    explicit SharpeRatio(double risk_free_rate = 0.02,
                        int periods_per_year = 252,
                        bool use_log_returns = false,
                        const std::string& name = "SharpeRatio")
        : AnalyzerBase(name),
          risk_free_rate_(risk_free_rate),
          periods_per_year_(periods_per_year),
          initial_value_(0.0),
          use_log_returns_(use_log_returns) {
        
        stats_.risk_free_rate = risk_free_rate;
        stats_.periods_per_year = periods_per_year;
    }
    
    /**
     * @brief Initialize analyzer
     */
    void start() override {
        AnalyzerBase::start();
        stats_.reset();
        stats_.risk_free_rate = risk_free_rate_;
        stats_.periods_per_year = periods_per_year_;
        returns_.clear();
        portfolio_values_.clear();
        benchmark_returns_.clear();
        initial_value_ = 0.0;
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
                double return_val;
                if (use_log_returns_) {
                    return_val = std::log(portfolio_value / prev_value);
                } else {
                    return_val = (portfolio_value - prev_value) / prev_value;
                }
                returns_.push_back(return_val);
                
                // Update period count
                stats_.total_periods = static_cast<int>(returns_.size());
                stats_.time_period_years = static_cast<double>(stats_.total_periods) / periods_per_year_;
            }
        }
        
        // Process benchmark return if provided
        if (!isNaN(benchmark_value)) {
            static double prev_benchmark = NaN;
            if (!isNaN(prev_benchmark) && prev_benchmark != 0.0) {
                double benchmark_return;
                if (use_log_returns_) {
                    benchmark_return = std::log(benchmark_value / prev_benchmark);
                } else {
                    benchmark_return = (benchmark_value - prev_benchmark) / prev_benchmark;
                }
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
        stats_.periods_per_year = periods;
    }
    
    /**
     * @brief Set maximum drawdown for Calmar ratio calculation
     */
    void setMaxDrawdown(double max_drawdown) {
        if (max_drawdown != 0.0) {
            stats_.calmar_ratio = stats_.portfolio_return / std::abs(max_drawdown);
        }
    }
    
    /**
     * @brief Get Sharpe ratio statistics
     */
    const SharpeStats& getStats() const {
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
        
        result["sharpe_ratio"] = stats_.sharpe_ratio;
        result["sharpe_ratio_daily"] = stats_.sharpe_ratio_daily;
        result["sharpe_ratio_monthly"] = stats_.sharpe_ratio_monthly;
        result["portfolio_return"] = stats_.portfolio_return;
        result["portfolio_volatility"] = stats_.portfolio_volatility;
        result["risk_free_rate"] = stats_.risk_free_rate;
        result["excess_return"] = stats_.excess_return;
        result["average_return"] = stats_.average_return;
        result["return_std"] = stats_.return_std;
        result["geometric_return"] = stats_.geometric_return;
        result["downside_volatility"] = stats_.downside_volatility;
        result["upside_volatility"] = stats_.upside_volatility;
        result["sortino_ratio"] = stats_.sortino_ratio;
        result["calmar_ratio"] = stats_.calmar_ratio;
        result["treynor_ratio"] = stats_.treynor_ratio;
        result["information_ratio"] = stats_.information_ratio;
        result["total_periods"] = stats_.total_periods;
        result["time_period_years"] = stats_.time_period_years;
        result["skewness"] = stats_.skewness;
        result["kurtosis"] = stats_.kurtosis;
        result["var_95"] = stats_.var_95;
        result["cvar_95"] = stats_.cvar_95;
        
        return result;
    }
    
    /**
     * @brief Print analysis results
     */
    void printAnalysis() const override {
        std::cout << "=== Sharpe Ratio Analysis Results ===" << std::endl;
        std::cout << "Sharpe Ratio (Annualized): " << stats_.sharpe_ratio << std::endl;
        std::cout << "Quality Assessment: " << stats_.getQualityAssessment() << std::endl;
        std::cout << std::endl;
        
        std::cout << "Portfolio Metrics:" << std::endl;
        std::cout << "  Annual Return: " << (stats_.portfolio_return * 100) << "%" << std::endl;
        std::cout << "  Annual Volatility: " << (stats_.portfolio_volatility * 100) << "%" << std::endl;
        std::cout << "  Excess Return: " << (stats_.excess_return * 100) << "%" << std::endl;
        std::cout << "  Risk-Free Rate: " << (stats_.risk_free_rate * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Alternative Risk Ratios:" << std::endl;
        std::cout << "  Sortino Ratio: " << stats_.sortino_ratio << std::endl;
        std::cout << "  Calmar Ratio: " << stats_.calmar_ratio << std::endl;
        std::cout << "  Information Ratio: " << stats_.information_ratio << std::endl;
        std::cout << std::endl;
        
        std::cout << "Return Distribution:" << std::endl;
        std::cout << "  Average Return: " << (stats_.average_return * 100) << "%" << std::endl;
        std::cout << "  Return Std Dev: " << (stats_.return_std * 100) << "%" << std::endl;
        std::cout << "  Skewness: " << stats_.skewness << std::endl;
        std::cout << "  Excess Kurtosis: " << stats_.kurtosis << std::endl;
        std::cout << "  Normal Distribution: " << (stats_.isNormallyDistributed() ? "Yes" : "No") << std::endl;
        std::cout << std::endl;
        
        std::cout << "Risk Metrics:" << std::endl;
        std::cout << "  VaR (95%): " << (stats_.var_95 * 100) << "%" << std::endl;
        std::cout << "  CVaR (95%): " << (stats_.cvar_95 * 100) << "%" << std::endl;
        std::cout << "  Downside Volatility: " << (stats_.downside_volatility * 100) << "%" << std::endl;
        std::cout << "  Upside Volatility: " << (stats_.upside_volatility * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Time Metrics:" << std::endl;
        std::cout << "  Total Periods: " << stats_.total_periods << std::endl;
        std::cout << "  Time Period: " << stats_.time_period_years << " years" << std::endl;
        std::cout << "  Periods Per Year: " << stats_.periods_per_year << std::endl;
    }

private:
    void calculateMetrics() {
        if (returns_.empty()) return;
        
        // Basic statistics
        stats_.average_return = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        
        // Standard deviation
        double variance = 0.0;
        for (double ret : returns_) {
            variance += std::pow(ret - stats_.average_return, 2);
        }
        variance /= (returns_.size() - 1);
        stats_.return_std = std::sqrt(variance);
        
        // Annualized metrics
        stats_.portfolio_return = stats_.average_return * periods_per_year_;
        stats_.portfolio_volatility = stats_.return_std * std::sqrt(periods_per_year_);
        
        // Geometric return
        double product = 1.0;
        for (double ret : returns_) {
            if (use_log_returns_) {
                product *= std::exp(ret);
            } else {
                product *= (1.0 + ret);
            }
        }
        stats_.geometric_return = std::pow(product, periods_per_year_ / static_cast<double>(returns_.size())) - 1.0;
        
        // Excess return and Sharpe ratio
        double rf_period = risk_free_rate_ / periods_per_year_;
        stats_.excess_return = stats_.portfolio_return - risk_free_rate_;
        
        if (stats_.portfolio_volatility != 0.0) {
            stats_.sharpe_ratio = stats_.excess_return / stats_.portfolio_volatility;
            stats_.sharpe_ratio_daily = (stats_.average_return - rf_period) / stats_.return_std;
        }
        
        // Monthly Sharpe (if applicable)
        if (periods_per_year_ >= 12) {
            double monthly_vol = stats_.return_std * std::sqrt(periods_per_year_ / 12.0);
            double monthly_excess = (stats_.average_return * periods_per_year_ - risk_free_rate_) / 12.0;
            stats_.sharpe_ratio_monthly = (monthly_vol != 0.0) ? monthly_excess / monthly_vol : 0.0;
        }
        
        // Downside and upside volatility
        calculateDirectionalVolatilities(rf_period);
        
        // Sortino ratio
        if (stats_.downside_volatility != 0.0) {
            stats_.sortino_ratio = stats_.excess_return / (stats_.downside_volatility * std::sqrt(periods_per_year_));
        }
        
        // Distribution statistics
        calculateDistributionStats();
        
        // Information ratio (vs benchmark if available)
        if (!benchmark_returns_.empty() && benchmark_returns_.size() == returns_.size()) {
            calculateInformationRatio();
        }
        
        // Risk metrics
        calculateRiskMetrics();
    }
    
    void calculateDirectionalVolatilities(double rf_period) {
        std::vector<double> downside_returns;
        std::vector<double> upside_returns;
        
        for (double ret : returns_) {
            if (ret < rf_period) {
                downside_returns.push_back(ret - rf_period);
            } else {
                upside_returns.push_back(ret - rf_period);
            }
        }
        
        // Downside volatility
        if (downside_returns.size() > 1) {
            double downside_variance = 0.0;
            for (double ret : downside_returns) {
                downside_variance += ret * ret;
            }
            downside_variance /= downside_returns.size();
            stats_.downside_volatility = std::sqrt(downside_variance);
        }
        
        // Upside volatility
        if (upside_returns.size() > 1) {
            double upside_variance = 0.0;
            for (double ret : upside_returns) {
                upside_variance += ret * ret;
            }
            upside_variance /= upside_returns.size();
            stats_.upside_volatility = std::sqrt(upside_variance);
        }
    }
    
    void calculateDistributionStats() {
        if (returns_.size() < 3) return;
        
        double mean = stats_.average_return;
        double std_dev = stats_.return_std;
        
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
    
    void calculateInformationRatio() {
        std::vector<double> excess_returns;
        for (size_t i = 0; i < std::min(returns_.size(), benchmark_returns_.size()); ++i) {
            excess_returns.push_back(returns_[i] - benchmark_returns_[i]);
        }
        
        if (!excess_returns.empty()) {
            double mean_excess = std::accumulate(excess_returns.begin(), excess_returns.end(), 0.0) / excess_returns.size();
            
            double tracking_error = 0.0;
            for (double excess : excess_returns) {
                tracking_error += std::pow(excess - mean_excess, 2);
            }
            tracking_error = std::sqrt(tracking_error / (excess_returns.size() - 1));
            
            if (tracking_error != 0.0) {
                stats_.information_ratio = (mean_excess * periods_per_year_) / (tracking_error * std::sqrt(periods_per_year_));
            }
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
};

} // namespace backtrader