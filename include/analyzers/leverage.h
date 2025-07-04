#pragma once

#include "../analyzer.h"
#include <vector>
#include <map>

namespace backtrader {
namespace analyzers {

/**
 * Leverage - Leverage analyzer
 * 
 * Analyzes the leverage usage of the strategy over time.
 * Tracks maximum, minimum, and average leverage ratios.
 */
class Leverage : public Analyzer {
public:
    // Parameters structure
    struct Params {
        bool fund;  // Fund mode calculation
        
        Params() : fund(false) {}
    };

    Leverage();
    Leverage(const Params& params);
    virtual ~Leverage() = default;

    // Analyzer interface
    void start() override;
    void next() override;
    void stop() override;
    AnalysisResult get_analysis() const override;

    // Leverage metrics
    double get_current_leverage() const;
    double get_max_leverage() const;
    double get_min_leverage() const;
    double get_average_leverage() const;
    
    // Historical data
    const std::vector<double>& get_leverage_history() const;
    std::map<std::string, double> get_leverage_stats() const;

private:
    // Parameters
    Params params_;
    
    // Leverage tracking
    std::vector<double> leverage_history_;
    double current_leverage_ = 0.0;
    double max_leverage_ = 0.0;
    double min_leverage_ = std::numeric_limits<double>::max();
    double sum_leverage_ = 0.0;
    int observation_count_ = 0;
    
    // Internal methods
    void calculate_current_leverage();
    void update_leverage_statistics(double leverage);
    
    // Calculation helpers
    double get_portfolio_value() const;
    double get_gross_exposure() const;
    double get_net_exposure() const;
    
    // Utility methods
    bool is_valid_leverage(double leverage) const;
    void reset_statistics();
};

} // namespace analyzers
} // namespace backtrader