#pragma once

#include "analyzers/AnalyzerBase.h"
#include <memory>
#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>

namespace backtrader {

/**
 * @brief Drawdown analysis for portfolio performance
 * 
 * Tracks maximum drawdown, current drawdown, drawdown duration,
 * and provides detailed drawdown statistics for risk assessment.
 */
class DrawdownAnalyzer : public AnalyzerBase {
public:
    /**
     * @brief Drawdown statistics structure
     */
    struct DrawdownStats {
        // Current drawdown
        double current_drawdown = 0.0;           // Current drawdown percentage
        double current_drawdown_value = 0.0;     // Current drawdown in currency units
        int current_drawdown_duration = 0;       // Current drawdown duration in periods
        
        // Maximum drawdown
        double max_drawdown = 0.0;               // Maximum drawdown percentage
        double max_drawdown_value = 0.0;         // Maximum drawdown in currency units
        int max_drawdown_duration = 0;           // Maximum drawdown duration
        
        // Drawdown periods
        double peak_value = 0.0;                 // Current peak portfolio value
        double valley_value = 0.0;               // Current valley (lowest point in drawdown)
        int peak_period = 0;                     // Period when peak occurred
        int valley_period = 0;                   // Period when valley occurred
        
        // Recovery statistics
        int recovery_periods = 0;                // Periods to recover from max drawdown
        double recovery_factor = 0.0;            // Recovery factor (gain needed to recover)
        
        // Drawdown frequency
        int total_drawdown_periods = 0;          // Total periods in drawdown
        int drawdown_episodes = 0;               // Number of drawdown episodes
        double avg_drawdown_duration = 0.0;      // Average drawdown duration
        double avg_drawdown_depth = 0.0;         // Average drawdown depth
        
        // Ulcer Index (measure of downside volatility)
        double ulcer_index = 0.0;
        
        // Calmar Ratio (annual return / max drawdown)
        double calmar_ratio = 0.0;
        
        // Risk metrics
        double var_95 = 0.0;                     // 95% Value at Risk
        double cvar_95 = 0.0;                    // 95% Conditional Value at Risk
        
        /**
         * @brief Check if currently in drawdown
         */
        bool isInDrawdown() const {
            return current_drawdown > 0.0;
        }
        
        /**
         * @brief Get risk-adjusted return metric
         */
        double getRiskAdjustedReturn(double annual_return) const {
            return (max_drawdown != 0.0) ? annual_return / max_drawdown : 0.0;
        }
        
        /**
         * @brief Get drawdown recovery time ratio
         */
        double getRecoveryTimeRatio() const {
            return (max_drawdown_duration > 0) ? 
                static_cast<double>(recovery_periods) / max_drawdown_duration : 0.0;
        }
        
        /**
         * @brief Reset all statistics
         */
        void reset() {
            *this = DrawdownStats{};
        }
    };

private:
    DrawdownStats stats_;
    std::vector<double> portfolio_values_;
    std::vector<double> drawdown_series_;
    std::vector<double> returns_;
    
    int current_period_;
    bool in_drawdown_;
    int drawdown_start_period_;
    double drawdown_start_value_;

public:
    explicit DrawdownAnalyzer(const std::string& name = "DrawdownAnalyzer")
        : AnalyzerBase(name),
          current_period_(0),
          in_drawdown_(false),
          drawdown_start_period_(0),
          drawdown_start_value_(0.0) {}
    
    /**
     * @brief Initialize analyzer
     */
    void start() override {
        AnalyzerBase::start();
        stats_.reset();
        portfolio_values_.clear();
        drawdown_series_.clear();
        returns_.clear();
        current_period_ = 0;
        in_drawdown_ = false;
        drawdown_start_period_ = 0;
        drawdown_start_value_ = 0.0;
    }
    
    /**
     * @brief Process new portfolio value
     * @param portfolio_value Current portfolio value
     */
    void updatePortfolioValue(double portfolio_value) {
        current_period_++;
        portfolio_values_.push_back(portfolio_value);
        
        // Calculate return if we have previous value
        if (portfolio_values_.size() > 1) {
            double prev_value = portfolio_values_[portfolio_values_.size() - 2];
            if (prev_value != 0.0) {
                double return_pct = (portfolio_value - prev_value) / prev_value;
                returns_.push_back(return_pct);
            }
        }
        
        // Update peak if new high
        if (portfolio_value > stats_.peak_value) {
            // New peak reached
            if (in_drawdown_) {
                // End of drawdown period
                endDrawdownPeriod();
            }
            
            stats_.peak_value = portfolio_value;
            stats_.peak_period = current_period_;
            stats_.current_drawdown = 0.0;
            stats_.current_drawdown_value = 0.0;
            stats_.current_drawdown_duration = 0;
        } else if (stats_.peak_value > 0.0) {
            // Calculate current drawdown
            stats_.current_drawdown_value = stats_.peak_value - portfolio_value;
            stats_.current_drawdown = (stats_.current_drawdown_value / stats_.peak_value) * 100.0;
            
            if (!in_drawdown_ && stats_.current_drawdown > 0.0) {
                // Start of new drawdown
                startDrawdownPeriod();
            }
            
            if (in_drawdown_) {
                stats_.current_drawdown_duration = current_period_ - drawdown_start_period_ + 1;
                
                // Update valley if new low in current drawdown
                if (portfolio_value < stats_.valley_value) {
                    stats_.valley_value = portfolio_value;
                    stats_.valley_period = current_period_;
                }
                
                // Update maximum drawdown if current is worse
                if (stats_.current_drawdown > stats_.max_drawdown) {
                    stats_.max_drawdown = stats_.current_drawdown;
                    stats_.max_drawdown_value = stats_.current_drawdown_value;
                    stats_.max_drawdown_duration = stats_.current_drawdown_duration;
                }
            }
        }
        
        // Store drawdown value for series analysis
        drawdown_series_.push_back(stats_.current_drawdown);
        
        // Update Ulcer Index
        updateUlcerIndex();
        
        // Update risk metrics
        updateRiskMetrics();
    }
    
    /**
     * @brief Get drawdown statistics
     */
    const DrawdownStats& getStats() const {
        return stats_;
    }
    
    /**
     * @brief Get analysis results as key-value pairs
     */
    std::map<std::string, double> getAnalysis() const override {
        std::map<std::string, double> result;
        
        result["current_drawdown"] = stats_.current_drawdown;
        result["current_drawdown_value"] = stats_.current_drawdown_value;
        result["current_drawdown_duration"] = stats_.current_drawdown_duration;
        
        result["max_drawdown"] = stats_.max_drawdown;
        result["max_drawdown_value"] = stats_.max_drawdown_value;
        result["max_drawdown_duration"] = stats_.max_drawdown_duration;
        
        result["peak_value"] = stats_.peak_value;
        result["valley_value"] = stats_.valley_value;
        result["recovery_periods"] = stats_.recovery_periods;
        result["recovery_factor"] = stats_.recovery_factor;
        
        result["total_drawdown_periods"] = stats_.total_drawdown_periods;
        result["drawdown_episodes"] = stats_.drawdown_episodes;
        result["avg_drawdown_duration"] = stats_.avg_drawdown_duration;
        result["avg_drawdown_depth"] = stats_.avg_drawdown_depth;
        
        result["ulcer_index"] = stats_.ulcer_index;
        result["calmar_ratio"] = stats_.calmar_ratio;
        result["var_95"] = stats_.var_95;
        result["cvar_95"] = stats_.cvar_95;
        
        result["is_in_drawdown"] = stats_.isInDrawdown() ? 1.0 : 0.0;
        
        return result;
    }
    
    /**
     * @brief Print analysis results
     */
    void printAnalysis() const override {
        std::cout << "=== Drawdown Analysis Results ===" << std::endl;
        std::cout << "Current Drawdown: " << stats_.current_drawdown << "%" << std::endl;
        std::cout << "Current Drawdown Duration: " << stats_.current_drawdown_duration << " periods" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Maximum Drawdown: " << stats_.max_drawdown << "%" << std::endl;
        std::cout << "Max Drawdown Value: " << stats_.max_drawdown_value << std::endl;
        std::cout << "Max Drawdown Duration: " << stats_.max_drawdown_duration << " periods" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Peak Portfolio Value: " << stats_.peak_value << std::endl;
        std::cout << "Valley Portfolio Value: " << stats_.valley_value << std::endl;
        std::cout << "Recovery Periods: " << stats_.recovery_periods << std::endl;
        std::cout << "Recovery Factor: " << (stats_.recovery_factor * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Total Drawdown Periods: " << stats_.total_drawdown_periods << std::endl;
        std::cout << "Drawdown Episodes: " << stats_.drawdown_episodes << std::endl;
        std::cout << "Average Drawdown Duration: " << stats_.avg_drawdown_duration << " periods" << std::endl;
        std::cout << "Average Drawdown Depth: " << stats_.avg_drawdown_depth << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Ulcer Index: " << stats_.ulcer_index << std::endl;
        std::cout << "Calmar Ratio: " << stats_.calmar_ratio << std::endl;
        std::cout << "VaR (95%): " << (stats_.var_95 * 100) << "%" << std::endl;
        std::cout << "CVaR (95%): " << (stats_.cvar_95 * 100) << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Currently in Drawdown: " << (stats_.isInDrawdown() ? "Yes" : "No") << std::endl;
    }
    
    /**
     * @brief Get drawdown time series
     */
    const std::vector<double>& getDrawdownSeries() const {
        return drawdown_series_;
    }
    
    /**
     * @brief Get portfolio value series
     */
    const std::vector<double>& getPortfolioValues() const {
        return portfolio_values_;
    }
    
    /**
     * @brief Calculate underwater curve (drawdown over time)
     */
    std::vector<double> getUnderwaterCurve() const {
        return drawdown_series_;  // Drawdown series is the underwater curve
    }
    
    /**
     * @brief Set annual return for Calmar ratio calculation
     */
    void setAnnualReturn(double annual_return) {
        stats_.calmar_ratio = (stats_.max_drawdown != 0.0) ? 
            annual_return / stats_.max_drawdown : 0.0;
    }

private:
    void startDrawdownPeriod() {
        in_drawdown_ = true;
        drawdown_start_period_ = current_period_;
        drawdown_start_value_ = stats_.peak_value;
        stats_.valley_value = portfolio_values_.back();
        stats_.valley_period = current_period_;
        stats_.drawdown_episodes++;
    }
    
    void endDrawdownPeriod() {
        if (in_drawdown_) {
            in_drawdown_ = false;
            int drawdown_duration = current_period_ - drawdown_start_period_;
            stats_.total_drawdown_periods += drawdown_duration;
            
            // Calculate recovery periods
            stats_.recovery_periods = current_period_ - stats_.valley_period;
            
            // Calculate recovery factor
            if (stats_.valley_value > 0.0) {
                stats_.recovery_factor = (stats_.peak_value - stats_.valley_value) / stats_.valley_value;
            }
            
            // Update averages
            if (stats_.drawdown_episodes > 0) {
                stats_.avg_drawdown_duration = static_cast<double>(stats_.total_drawdown_periods) / 
                                             stats_.drawdown_episodes;
            }
        }
    }
    
    void updateUlcerIndex() {
        if (drawdown_series_.size() < 2) {
            return;
        }
        
        // Calculate Ulcer Index as RMS of drawdowns
        double sum_squared_drawdowns = 0.0;
        for (double dd : drawdown_series_) {
            sum_squared_drawdowns += dd * dd;
        }
        
        stats_.ulcer_index = std::sqrt(sum_squared_drawdowns / drawdown_series_.size());
    }
    
    void updateRiskMetrics() {
        if (returns_.size() < 20) {  // Need at least 20 observations
            return;
        }
        
        // Sort returns for VaR calculation
        std::vector<double> sorted_returns = returns_;
        std::sort(sorted_returns.begin(), sorted_returns.end());
        
        // Calculate 95% VaR (5th percentile)
        size_t var_index = static_cast<size_t>(sorted_returns.size() * 0.05);
        if (var_index < sorted_returns.size()) {
            stats_.var_95 = sorted_returns[var_index];
            
            // Calculate 95% CVaR (average of returns below VaR)
            double sum_tail_returns = 0.0;
            int tail_count = 0;
            for (size_t i = 0; i <= var_index; ++i) {
                sum_tail_returns += sorted_returns[i];
                tail_count++;
            }
            
            if (tail_count > 0) {
                stats_.cvar_95 = sum_tail_returns / tail_count;
            }
        }
        
        // Update average drawdown depth
        if (!drawdown_series_.empty()) {
            double sum_drawdowns = 0.0;
            int positive_drawdowns = 0;
            
            for (double dd : drawdown_series_) {
                if (dd > 0.0) {
                    sum_drawdowns += dd;
                    positive_drawdowns++;
                }
            }
            
            if (positive_drawdowns > 0) {
                stats_.avg_drawdown_depth = sum_drawdowns / positive_drawdowns;
            }
        }
    }
};

} // namespace backtrader