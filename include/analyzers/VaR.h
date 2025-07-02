#pragma once

#include "analyzers/AnalyzerBase.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace backtrader {

/**
 * @brief Value at Risk (VaR) analyzer
 * 
 * VaR measures the potential loss that could happen in a portfolio over a defined period
 * for a given confidence interval. It answers: "What is the maximum loss we could face
 * with X% confidence over the next N days?"
 */
class VaR : public AnalyzerBase {
public:
    /**
     * @brief VaR statistics structure
     */
    struct VaRStats {
        // Basic VaR metrics
        double var_95 = 0.0;                    // 95% VaR
        double var_99 = 0.0;                    // 99% VaR
        double var_99_9 = 0.0;                  // 99.9% VaR
        double cvar_95 = 0.0;                   // 95% Conditional VaR (Expected Shortfall)
        double cvar_99 = 0.0;                   // 99% Conditional VaR
        double cvar_99_9 = 0.0;                 // 99.9% Conditional VaR
        
        // Risk metrics
        double maximum_loss = 0.0;              // Maximum single-period loss
        double average_loss = 0.0;              // Average loss (only negative returns)
        double loss_volatility = 0.0;           // Volatility of losses
        double tail_ratio = 0.0;                // Average tail loss / VaR
        
        // Distribution metrics
        double return_volatility = 0.0;         // Overall return volatility
        double skewness = 0.0;                  // Return distribution skewness
        double kurtosis = 0.0;                  // Return distribution excess kurtosis
        double jarque_bera = 0.0;               // Jarque-Bera normality test statistic
        
        // Time-based metrics
        int total_periods = 0;                  // Total periods analyzed
        int loss_periods = 0;                   // Periods with losses
        double loss_frequency = 0.0;            // Frequency of losses
        double var_breaches_95 = 0.0;           // Number of times 95% VaR was exceeded
        double var_breaches_99 = 0.0;           // Number of times 99% VaR was exceeded
        
        // Model validation
        double kupiec_test_95 = 0.0;            // Kupiec test for 95% VaR
        double kupiec_test_99 = 0.0;            // Kupiec test for 99% VaR
        bool model_valid_95 = false;           // Whether 95% VaR model is valid
        bool model_valid_99 = false;           // Whether 99% VaR model is valid
        
        // Scaling factors
        double daily_var_95 = 0.0;             // Daily VaR (if data is not daily)
        double monthly_var_95 = 0.0;           // Monthly VaR
        double annual_var_95 = 0.0;            // Annual VaR
        
        /**
         * @brief Get risk assessment
         */
        std::string getRiskAssessment() const {
            if (var_95 > -0.05) return "Low Risk";
            else if (var_95 > -0.10) return "Moderate Risk";
            else if (var_95 > -0.20) return "High Risk";
            else return "Very High Risk";
        }
        
        /**
         * @brief Get tail risk ratio
         */
        double getTailRiskRatio() const {
            return (var_95 != 0.0) ? cvar_95 / var_95 : 0.0;
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
            *this = VaRStats{};
        }
    };

private:
    VaRStats stats_;
    std::vector<double> returns_;
    std::vector<double> portfolio_values_;
    
    double confidence_95_;
    double confidence_99_;
    double confidence_99_9_;
    int periods_per_year_;
    double initial_value_;
    
public:
    explicit VaR(double confidence_95 = 0.95,
                 double confidence_99 = 0.99,
                 double confidence_99_9 = 0.999,
                 int periods_per_year = 252,
                 const std::string& name = "VaR")
        : AnalyzerBase(name),
          confidence_95_(confidence_95),
          confidence_99_(confidence_99),
          confidence_99_9_(confidence_99_9),
          periods_per_year_(periods_per_year),
          initial_value_(0.0) {}
    
    /**
     * @brief Initialize analyzer
     */
    void start() override {
        AnalyzerBase::start();
        stats_.reset();
        returns_.clear();
        portfolio_values_.clear();
        initial_value_ = 0.0;
    }
    
    /**
     * @brief Process new portfolio value
     */
    void updatePortfolioValue(double portfolio_value) {
        portfolio_values_.push_back(portfolio_value);
        
        if (initial_value_ == 0.0) {
            initial_value_ = portfolio_value;
            return;
        }
        
        // Calculate return
        if (portfolio_values_.size() > 1) {
            double prev_value = portfolio_values_[portfolio_values_.size() - 2];
            if (prev_value != 0.0) {
                double return_val = (portfolio_value - prev_value) / prev_value;
                returns_.push_back(return_val);
                
                stats_.total_periods = static_cast<int>(returns_.size());
                
                // Calculate metrics if we have enough data
                if (returns_.size() >= 20) {
                    calculateMetrics();
                }
            }
        }
    }
    
    /**
     * @brief Set confidence levels
     */
    void setConfidenceLevels(double conf_95, double conf_99, double conf_99_9) {
        confidence_95_ = conf_95;
        confidence_99_ = conf_99;
        confidence_99_9_ = conf_99_9;
    }
    
    /**
     * @brief Get VaR statistics
     */
    const VaRStats& getStats() const {
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
        
        result["var_95"] = stats_.var_95;
        result["var_99"] = stats_.var_99;
        result["var_99_9"] = stats_.var_99_9;
        result["cvar_95"] = stats_.cvar_95;
        result["cvar_99"] = stats_.cvar_99;
        result["cvar_99_9"] = stats_.cvar_99_9;
        result["maximum_loss"] = stats_.maximum_loss;
        result["average_loss"] = stats_.average_loss;
        result["loss_volatility"] = stats_.loss_volatility;
        result["tail_ratio"] = stats_.tail_ratio;
        result["return_volatility"] = stats_.return_volatility;
        result["skewness"] = stats_.skewness;
        result["kurtosis"] = stats_.kurtosis;
        result["jarque_bera"] = stats_.jarque_bera;
        result["total_periods"] = stats_.total_periods;
        result["loss_periods"] = stats_.loss_periods;
        result["loss_frequency"] = stats_.loss_frequency;
        result["var_breaches_95"] = stats_.var_breaches_95;
        result["var_breaches_99"] = stats_.var_breaches_99;
        result["kupiec_test_95"] = stats_.kupiec_test_95;
        result["kupiec_test_99"] = stats_.kupiec_test_99;
        result["daily_var_95"] = stats_.daily_var_95;
        result["monthly_var_95"] = stats_.monthly_var_95;
        result["annual_var_95"] = stats_.annual_var_95;
        
        return result;
    }
    
    /**
     * @brief Print analysis results
     */
    void printAnalysis() const override {
        std::cout << "=== Value at Risk Analysis Results ===" << std::endl;
        std::cout << "Risk Assessment: " << stats_.getRiskAssessment() << std::endl;
        std::cout << std::endl;
        
        std::cout << "VaR Metrics:" << std::endl;
        std::cout << "  95% VaR: " << (stats_.var_95 * 100) << "%" << std::endl;
        std::cout << "  99% VaR: " << (stats_.var_99 * 100) << "%" << std::endl;
        std::cout << "  99.9% VaR: " << (stats_.var_99_9 * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Conditional VaR (Expected Shortfall):" << std::endl;
        std::cout << "  95% CVaR: " << (stats_.cvar_95 * 100) << "%" << std::endl;
        std::cout << "  99% CVaR: " << (stats_.cvar_99 * 100) << "%" << std::endl;
        std::cout << "  99.9% CVaR: " << (stats_.cvar_99_9 * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Risk Metrics:" << std::endl;
        std::cout << "  Maximum Loss: " << (stats_.maximum_loss * 100) << "%" << std::endl;
        std::cout << "  Average Loss: " << (stats_.average_loss * 100) << "%" << std::endl;
        std::cout << "  Tail Risk Ratio: " << stats_.getTailRiskRatio() << std::endl;
        std::cout << "  Return Volatility: " << (stats_.return_volatility * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Distribution Analysis:" << std::endl;
        std::cout << "  Skewness: " << stats_.skewness << std::endl;
        std::cout << "  Excess Kurtosis: " << stats_.kurtosis << std::endl;
        std::cout << "  Jarque-Bera Test: " << stats_.jarque_bera << std::endl;
        std::cout << "  Normal Distribution: " << (stats_.isNormallyDistributed() ? "Yes" : "No") << std::endl;
        std::cout << std::endl;
        
        std::cout << "Model Validation:" << std::endl;
        std::cout << "  VaR Breaches (95%): " << stats_.var_breaches_95 << std::endl;
        std::cout << "  VaR Breaches (99%): " << stats_.var_breaches_99 << std::endl;
        std::cout << "  Model Valid (95%): " << (stats_.model_valid_95 ? "Yes" : "No") << std::endl;
        std::cout << "  Model Valid (99%): " << (stats_.model_valid_99 ? "Yes" : "No") << std::endl;
        std::cout << std::endl;
        
        std::cout << "Time-Scaled VaR (95%):" << std::endl;
        std::cout << "  Daily VaR: " << (stats_.daily_var_95 * 100) << "%" << std::endl;
        std::cout << "  Monthly VaR: " << (stats_.monthly_var_95 * 100) << "%" << std::endl;
        std::cout << "  Annual VaR: " << (stats_.annual_var_95 * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Loss Statistics:" << std::endl;
        std::cout << "  Total Periods: " << stats_.total_periods << std::endl;
        std::cout << "  Loss Periods: " << stats_.loss_periods << std::endl;
        std::cout << "  Loss Frequency: " << (stats_.loss_frequency * 100) << "%" << std::endl;
    }

private:
    void calculateMetrics() {
        if (returns_.empty()) return;
        
        // Sort returns for percentile calculations
        std::vector<double> sorted_returns = returns_;
        std::sort(sorted_returns.begin(), sorted_returns.end());
        
        // Calculate VaR at different confidence levels
        calculateVaR(sorted_returns);
        
        // Calculate CVaR (Expected Shortfall)
        calculateCVaR(sorted_returns);
        
        // Calculate loss statistics
        calculateLossStatistics();
        
        // Calculate distribution statistics
        calculateDistributionStats();
        
        // Model validation
        validateModel();
        
        // Time scaling
        calculateTimeScaledVaR();
    }
    
    void calculateVaR(const std::vector<double>& sorted_returns) {
        size_t n = sorted_returns.size();
        
        // 95% VaR
        size_t var_95_index = static_cast<size_t>(n * (1.0 - confidence_95_));
        if (var_95_index < n) {
            stats_.var_95 = sorted_returns[var_95_index];
        }
        
        // 99% VaR
        size_t var_99_index = static_cast<size_t>(n * (1.0 - confidence_99_));
        if (var_99_index < n) {
            stats_.var_99 = sorted_returns[var_99_index];
        }
        
        // 99.9% VaR
        size_t var_99_9_index = static_cast<size_t>(n * (1.0 - confidence_99_9_));
        if (var_99_9_index < n) {
            stats_.var_99_9 = sorted_returns[var_99_9_index];
        }
    }
    
    void calculateCVaR(const std::vector<double>& sorted_returns) {
        size_t n = sorted_returns.size();
        
        // 95% CVaR
        size_t cvar_95_index = static_cast<size_t>(n * (1.0 - confidence_95_));
        if (cvar_95_index < n) {
            double sum = 0.0;
            for (size_t i = 0; i <= cvar_95_index; ++i) {
                sum += sorted_returns[i];
            }
            stats_.cvar_95 = sum / (cvar_95_index + 1);
        }
        
        // 99% CVaR
        size_t cvar_99_index = static_cast<size_t>(n * (1.0 - confidence_99_));
        if (cvar_99_index < n) {
            double sum = 0.0;
            for (size_t i = 0; i <= cvar_99_index; ++i) {
                sum += sorted_returns[i];
            }
            stats_.cvar_99 = sum / (cvar_99_index + 1);
        }
        
        // 99.9% CVaR
        size_t cvar_99_9_index = static_cast<size_t>(n * (1.0 - confidence_99_9_));
        if (cvar_99_9_index < n) {
            double sum = 0.0;
            for (size_t i = 0; i <= cvar_99_9_index; ++i) {
                sum += sorted_returns[i];
            }
            stats_.cvar_99_9 = sum / (cvar_99_9_index + 1);
        }
    }
    
    void calculateLossStatistics() {
        std::vector<double> losses;
        for (double ret : returns_) {
            if (ret < 0.0) {
                losses.push_back(ret);
                stats_.loss_periods++;
            }
        }
        
        if (!losses.empty()) {
            // Maximum loss
            stats_.maximum_loss = *std::min_element(losses.begin(), losses.end());
            
            // Average loss
            double sum_losses = 0.0;
            for (double loss : losses) {
                sum_losses += loss;
            }
            stats_.average_loss = sum_losses / losses.size();
            
            // Loss volatility
            double variance = 0.0;
            for (double loss : losses) {
                variance += std::pow(loss - stats_.average_loss, 2);
            }
            stats_.loss_volatility = std::sqrt(variance / (losses.size() - 1));
            
            // Loss frequency
            stats_.loss_frequency = static_cast<double>(stats_.loss_periods) / stats_.total_periods;
            
            // Tail ratio
            stats_.tail_ratio = (stats_.var_95 != 0.0) ? stats_.cvar_95 / stats_.var_95 : 0.0;
        }
        
        // Overall return volatility
        if (returns_.size() > 1) {
            double mean_return = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
            double variance = 0.0;
            for (double ret : returns_) {
                variance += std::pow(ret - mean_return, 2);
            }
            stats_.return_volatility = std::sqrt(variance / (returns_.size() - 1));
        }
    }
    
    void calculateDistributionStats() {
        if (returns_.size() < 3) return;
        
        double mean = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        double variance = 0.0;
        double skew_sum = 0.0;
        double kurt_sum = 0.0;
        
        for (double ret : returns_) {
            double diff = ret - mean;
            variance += diff * diff;
            skew_sum += diff * diff * diff;
            kurt_sum += diff * diff * diff * diff;
        }
        
        variance /= (returns_.size() - 1);
        double std_dev = std::sqrt(variance);
        
        if (std_dev > 0.0) {
            stats_.skewness = (skew_sum / returns_.size()) / std::pow(std_dev, 3);
            stats_.kurtosis = (kurt_sum / returns_.size()) / std::pow(std_dev, 4) - 3.0;
            
            // Jarque-Bera test
            double n = static_cast<double>(returns_.size());
            stats_.jarque_bera = (n / 6.0) * (stats_.skewness * stats_.skewness + 
                                             (stats_.kurtosis * stats_.kurtosis) / 4.0);
        }
    }
    
    void validateModel() {
        // Count VaR breaches
        for (double ret : returns_) {
            if (ret <= stats_.var_95) {
                stats_.var_breaches_95++;
            }
            if (ret <= stats_.var_99) {
                stats_.var_breaches_99++;
            }
        }
        
        // Kupiec test for model validation
        double expected_breaches_95 = stats_.total_periods * (1.0 - confidence_95_);
        double expected_breaches_99 = stats_.total_periods * (1.0 - confidence_99_);
        
        if (expected_breaches_95 > 0.0) {
            double p_95 = stats_.var_breaches_95 / stats_.total_periods;
            stats_.kupiec_test_95 = -2.0 * std::log(std::pow((1.0 - confidence_95_), stats_.var_breaches_95) *
                                                   std::pow(confidence_95_, stats_.total_periods - stats_.var_breaches_95)) +
                                   2.0 * std::log(std::pow(p_95, stats_.var_breaches_95) *
                                                 std::pow(1.0 - p_95, stats_.total_periods - stats_.var_breaches_95));
            
            // Model is valid if test statistic < 3.84 (95% confidence)
            stats_.model_valid_95 = stats_.kupiec_test_95 < 3.84;
        }
        
        if (expected_breaches_99 > 0.0) {
            double p_99 = stats_.var_breaches_99 / stats_.total_periods;
            stats_.kupiec_test_99 = -2.0 * std::log(std::pow((1.0 - confidence_99_), stats_.var_breaches_99) *
                                                   std::pow(confidence_99_, stats_.total_periods - stats_.var_breaches_99)) +
                                   2.0 * std::log(std::pow(p_99, stats_.var_breaches_99) *
                                                 std::pow(1.0 - p_99, stats_.total_periods - stats_.var_breaches_99));
            
            stats_.model_valid_99 = stats_.kupiec_test_99 < 3.84;
        }
    }
    
    void calculateTimeScaledVaR() {
        // Scale VaR to different time horizons using square root of time
        double sqrt_periods_per_day = std::sqrt(static_cast<double>(periods_per_year_) / 252.0);
        double sqrt_periods_per_month = std::sqrt(static_cast<double>(periods_per_year_) / 12.0);
        double sqrt_periods_per_year = std::sqrt(static_cast<double>(periods_per_year_));
        
        stats_.daily_var_95 = stats_.var_95 / sqrt_periods_per_day;
        stats_.monthly_var_95 = stats_.var_95 * std::sqrt(12.0 / periods_per_year_);
        stats_.annual_var_95 = stats_.var_95 * std::sqrt(static_cast<double>(periods_per_year_));
    }
};

} // namespace backtrader