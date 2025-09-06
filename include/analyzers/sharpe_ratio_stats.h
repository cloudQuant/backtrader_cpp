#pragma once

#include "../analyzer.h"
#include <chrono>
#include <vector>
#include <cmath>
#include <memory>

namespace backtrader {
namespace analyzers {

/**
 * SharpeRatioStats analyzer - comprehensive Sharpe ratio statistical analysis
 * 
 * This analyzer provides advanced statistical analysis of Sharpe ratios including:
 * - Estimated Sharpe ratio calculation
 * - Annualized Sharpe ratio
 * - Sharpe ratio standard deviation
 * - Probabilistic Sharpe Ratio (PSR)
 * - Minimum Track Record Length (minTRL)
 * - Deflated Sharpe Ratio (DSR)
 * - Expected Maximum Sharpe Ratio
 * - Statistical significance testing
 * 
 * The implementation follows advanced portfolio theory and statistical methods
 * for evaluating the significance and reliability of Sharpe ratio measurements.
 * 
 * Parameters:
 *   - risk_free_rate: Risk-free rate for Sharpe calculation (default: 0.0)
 *   - benchmark_sharpe: Benchmark Sharpe ratio for comparison (default: 0.0)
 *   - confidence_level: Confidence level for statistical tests (default: 0.95)
 *   - annualization_factor: Factor for annualizing returns (default: 252 for daily)
 * 
 * Features:
 *   - Comprehensive Sharpe ratio analysis
 *   - Statistical significance testing
 *   - Multiple Sharpe ratio variants
 *   - Risk-adjusted performance metrics
 *   - Confidence intervals and hypothesis testing
 * 
 * get_analysis():
 *   - Returns comprehensive Sharpe ratio statistics
 *   - Includes all calculated metrics and statistical tests
 */
class SharpeRatioStats : public Analyzer {
public:
    // Parameters structure
    struct Params {
        double risk_free_rate = 0.0;           // Risk-free rate
        double benchmark_sharpe = 0.0;         // Benchmark Sharpe ratio
        double confidence_level = 0.95;        // Confidence level (0.0 to 1.0)
        double annualization_factor = 252.0;   // Annualization factor (252 for daily)
        bool calculate_psr = true;             // Calculate Probabilistic Sharpe Ratio
        bool calculate_dsr = true;             // Calculate Deflated Sharpe Ratio
        bool calculate_min_trl = true;         // Calculate Minimum Track Record Length
        bool calculate_confidence_intervals = true; // Calculate confidence intervals
    };
    
    SharpeRatioStats(const Params& params = Params{});
    virtual ~SharpeRatioStats() = default;
    
    // Override analyzer methods
    void start() override;
    void next() override;
    void stop() override;
    AnalysisResult get_analysis() const override;
    
    // Comprehensive statistics structure
    struct SharpeStatistics {
        // Basic Sharpe ratio metrics
        double sharpe_ratio = 0.0;                  // Standard Sharpe ratio
        double annualized_sharpe = 0.0;             // Annualized Sharpe ratio
        double sharpe_std = 0.0;                    // Sharpe ratio standard deviation
        
        // Advanced statistical metrics
        double probabilistic_sharpe_ratio = 0.0;    // PSR
        double deflated_sharpe_ratio = 0.0;         // DSR
        double minimum_track_record_length = 0.0;   // minTRL (in periods)
        double expected_max_sharpe = 0.0;           // Expected maximum Sharpe
        
        // Statistical significance
        double t_statistic = 0.0;                   // t-test statistic
        double p_value = 0.0;                       // p-value for significance
        bool is_significant = false;                // Statistical significance flag
        
        // Confidence intervals
        double sharpe_lower_ci = 0.0;               // Lower confidence interval
        double sharpe_upper_ci = 0.0;               // Upper confidence interval
        
        // Sample statistics
        size_t sample_size = 0;                     // Number of observations
        double mean_return = 0.0;                   // Mean return
        double return_std = 0.0;                    // Return standard deviation
        double skewness = 0.0;                      // Return skewness
        double kurtosis = 0.0;                      // Return excess kurtosis
        
        // Risk metrics
        double var_95 = 0.0;                        // 95% Value at Risk
        double cvar_95 = 0.0;                       // 95% Conditional VaR
        double maximum_drawdown = 0.0;              // Maximum drawdown
        
        // Performance attribution
        double information_ratio = 0.0;             // Information ratio vs benchmark
        double treynor_ratio = 0.0;                 // Treynor ratio
        double sortino_ratio = 0.0;                 // Sortino ratio
    };
    
    // Get comprehensive statistics
    SharpeStatistics get_sharpe_statistics() const;
    
    // Individual metric calculations
    double get_sharpe_ratio() const;
    double get_probabilistic_sharpe_ratio() const;
    double get_deflated_sharpe_ratio() const;
    double get_minimum_track_record_length() const;
    
    // Statistical tests
    bool test_sharpe_significance() const;
    std::pair<double, double> get_sharpe_confidence_interval() const;
    double calculate_t_statistic() const;
    double calculate_p_value() const;
    
    // Export functionality
    void export_statistics_csv(const std::string& filename) const;
    void export_returns_csv(const std::string& filename) const;
    
protected:
    Params p;  // Parameters
    
    // Return data storage
    std::vector<double> returns_;
    std::vector<std::chrono::system_clock::time_point> dates_;
    
    // Running calculations
    double portfolio_value_start_ = 0.0;
    double portfolio_value_previous_ = 0.0;
    
    // Cached statistics (mutable for lazy evaluation)
    mutable bool stats_cached_ = false;
    mutable SharpeStatistics cached_stats_;
    
private:
    // Core calculation methods
    double calculate_sharpe_ratio() const;
    double calculate_annualized_sharpe() const;
    double calculate_sharpe_std() const;
    
    // Advanced statistical methods
    double calculate_probabilistic_sharpe_ratio() const;
    double calculate_deflated_sharpe_ratio() const;
    double calculate_minimum_track_record_length() const;
    double calculate_expected_max_sharpe() const;
    
    // Statistical tests
    double calculate_t_statistic_internal() const;
    double calculate_p_value_internal() const;
    std::pair<double, double> calculate_confidence_interval() const;
    
    // Moment calculations
    double calculate_mean() const;
    double calculate_std() const;
    double calculate_skewness() const;
    double calculate_kurtosis() const;
    
    // Risk metrics
    double calculate_var_95() const;
    double calculate_cvar_95() const;
    double calculate_maximum_drawdown() const;
    
    // Additional ratios
    double calculate_information_ratio() const;
    double calculate_treynor_ratio() const;
    double calculate_sortino_ratio() const;
    
    // Utility methods
    double get_current_portfolio_value() const;
    std::chrono::system_clock::time_point get_current_datetime() const;
    
    // Statistical utility functions
    double normal_cdf(double x) const;
    double normal_pdf(double x) const;
    double student_t_cdf(double t, int df) const;
    double gamma_function(double x) const;
    
    // Cache management
    void invalidate_cache();
    void update_cache() const;
    
    // Error handling
    void validate_parameters() const;
    bool is_valid_return(double ret) const;
    
    // Formatting helpers
    std::string format_datetime(const std::chrono::system_clock::time_point& dt) const;
    std::string format_percentage(double value) const;
};

} // namespace analyzers
} // namespace backtrader

// Register the analyzer
REGISTER_ANALYZER(backtrader::analyzers::SharpeRatioStats);