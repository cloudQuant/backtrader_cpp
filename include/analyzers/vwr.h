#pragma once

#include "../analyzer.h"
#include <vector>
#include <cmath>

namespace backtrader {
namespace analyzers {

/**
 * VWR - Variability-Weighted Return analyzer
 * 
 * Calculates the Variability-Weighted Return, which is a risk-adjusted return metric
 * that penalizes volatility more heavily than the Sharpe ratio.
 * 
 * VWR = (Annualized Return) / (Volatility^2)
 */
class VWR : public Analyzer {
public:
    // Parameters structure
    struct Params {
        bool fund = false;           // Fund mode calculation
        double timereturn = false;   // Use time-based returns
        int periods_per_year = 252;  // Trading periods per year
    };

    VWR(const Params& params = Params{});
    virtual ~VWR() = default;

    // Analyzer interface
    void start() override;
    void next() override;
    void stop() override;
    AnalysisResult get_analysis() override;

    // VWR metrics
    double get_vwr() const;
    double get_annualized_return() const;
    double get_volatility() const;
    double get_total_return() const;
    
    // Supporting statistics
    double get_mean_return() const;
    double get_std_deviation() const;
    int get_observation_count() const;

private:
    // Parameters
    Params params_;
    
    // Return tracking
    std::vector<double> returns_;
    double previous_value_ = 0.0;
    double initial_value_ = 0.0;
    double current_value_ = 0.0;
    
    // Statistical accumulators
    double sum_returns_ = 0.0;
    double sum_squared_returns_ = 0.0;
    int observation_count_ = 0;
    
    // Calculated metrics
    double vwr_ = 0.0;
    double annualized_return_ = 0.0;
    double volatility_ = 0.0;
    
    // Internal methods
    void update_portfolio_value();
    void calculate_period_return();
    void update_statistics(double return_value);
    void calculate_final_metrics();
    
    // Statistical calculations
    double calculate_mean_return() const;
    double calculate_variance() const;
    double calculate_standard_deviation() const;
    double annualize_return(double return_value) const;
    double annualize_volatility(double vol) const;
    
    // VWR calculation
    double calculate_vwr() const;
    
    // Utility methods
    double get_broker_value() const;
    bool has_sufficient_data() const;
    bool is_valid_return(double return_value) const;
};

} // namespace analyzers
} // namespace backtrader