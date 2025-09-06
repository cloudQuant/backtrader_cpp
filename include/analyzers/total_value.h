#pragma once

#include "../analyzer.h"
#include <chrono>
#include <map>

namespace backtrader {
namespace analyzers {

/**
 * TotalValue analyzer - records total portfolio value at every time step
 * 
 * This analyzer captures the total portfolio value from the broker at each
 * time step, providing a complete time series of portfolio valuation.
 * 
 * The analyzer maintains an ordered dictionary with datetime keys and 
 * portfolio values, matching the Python implementation exactly.
 * 
 * Usage:
 *   - Automatically records broker.getvalue() at each next() call
 *   - Returns OrderedDict with datetime->value mappings
 *   - Useful for detailed portfolio tracking and analysis
 * 
 * get_analysis():
 *   - Returns OrderedDict<datetime, double> with complete value history
 */
class TotalValue : public Analyzer {
public:
    TotalValue();
    virtual ~TotalValue() = default;
    
    // Override analyzer methods
    void start() override;
    void next() override;
    AnalysisResult get_analysis() const override;
    
    // Convenience method to get raw data (matching Python interface)
    const OrderedDict<std::chrono::system_clock::time_point, double>& get_rets() const {
        return rets;
    }
    
    // Get specific value at datetime
    double get_value_at(const std::chrono::system_clock::time_point& dt) const;
    
    // Get value history as vector for analysis
    std::vector<double> get_value_history() const;
    std::vector<std::chrono::system_clock::time_point> get_datetime_history() const;
    
    // Statistics helpers
    double get_initial_value() const;
    double get_final_value() const;
    double get_total_return() const;
    double get_max_value() const;
    double get_min_value() const;
    
    // Export functionality
    void export_to_csv(const std::string& filename) const;
    
protected:
    // Main data storage - ordered dict of datetime->value pairs
    OrderedDict<std::chrono::system_clock::time_point, double> rets;
    
private:
    // Helper methods
    double get_current_broker_value() const;
    std::chrono::system_clock::time_point get_current_datetime() const;
    
    // Format datetime for output
    std::string format_datetime(const std::chrono::system_clock::time_point& dt) const;
    
    // Statistics caching (lazy evaluation)
    mutable bool stats_cached = false;
    mutable double cached_initial_value = 0.0;
    mutable double cached_final_value = 0.0;
    mutable double cached_max_value = 0.0;
    mutable double cached_min_value = 0.0;
    
    void update_stats_cache() const;
    void invalidate_stats_cache();
};

} // namespace analyzers
} // namespace backtrader

// Register the analyzer
REGISTER_ANALYZER(backtrader::analyzers::TotalValue);