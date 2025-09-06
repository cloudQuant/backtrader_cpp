#pragma once

#include "../observer.h"
#include <cmath>
#include <vector>

namespace backtrader {
namespace observers {

/**
 * LogReturns - Logarithmic returns observer
 * 
 * Calculates and tracks logarithmic returns of the portfolio value.
 * Log returns are useful for statistical analysis and risk metrics.
 */
class LogReturns : public Observer {
public:
    // Parameters structure
    struct Params {
        bool fund = false;           // Fund mode (different calculation)
        int lookback = 1;           // Lookback period for return calculation
        bool timereturn = false;    // Time-weighted returns
    };

    // Lines  
    enum Lines {
        LOGRETURNS = 0
    };

    LogReturns(const Params& params = Params{});
    virtual ~LogReturns() = default;

    // Observer interface
    void next() override;
    void start() override;
    void stop() override;

    // Return metrics
    double get_current_return() const;
    double get_cumulative_return() const;
    double get_annualized_return() const;
    double get_volatility() const;
    double get_sharpe_ratio(double risk_free_rate = 0.0) const;
    
    // Statistical metrics
    double get_mean_return() const;
    double get_std_deviation() const;
    double get_skewness() const;
    double get_kurtosis() const;
    
    // Risk metrics
    double get_var(double confidence = 0.05) const;  // Value at Risk
    double get_cvar(double confidence = 0.05) const; // Conditional VaR
    double get_maximum_return() const;
    double get_minimum_return() const;

private:
    // Parameters
    Params params_;
    
    // Return tracking
    std::vector<double> portfolio_values_;
    std::vector<double> log_returns_;
    double previous_value_ = 0.0;
    double initial_value_ = 0.0;
    
    // Statistical accumulators
    double sum_returns_ = 0.0;
    double sum_squared_returns_ = 0.0;
    double sum_cubed_returns_ = 0.0;
    double sum_quartic_returns_ = 0.0;
    
    // Internal methods
    void update_portfolio_value();
    void calculate_log_return();
    void update_statistics(double log_return);
    
    // Statistical calculations
    double calculate_mean() const;
    double calculate_variance() const;
    double calculate_standard_deviation() const;
    double calculate_skewness() const;
    double calculate_kurtosis() const;
    
    // Risk calculations
    double calculate_var(double confidence) const;
    double calculate_cvar(double confidence) const;
    
    // Utility methods
    double get_broker_value() const;
    double annualize_return(double return_value) const;
    double annualize_volatility(double volatility) const;
    int get_trading_periods_per_year() const;
    
    // Validation
    bool is_valid_portfolio_value(double value) const;
    bool has_sufficient_data() const;
};

} // namespace observers
} // namespace backtrader