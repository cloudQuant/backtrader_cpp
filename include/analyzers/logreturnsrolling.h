#pragma once

#include "../analyzer.h"
#include <chrono>
#include <deque>
#include <memory>
#include <cmath>

namespace backtrader {
namespace analyzers {

/**
 * LogReturnsRolling analyzer - calculates rolling logarithmic returns
 * 
 * This analyzer calculates rolling logarithmic returns over a specified timeframe
 * and compression period. It maintains a rolling window of historical values and
 * computes log returns between current and historical values.
 * 
 * The analyzer inherits from TimeFrameAnalyzerBase to handle timeframe-specific
 * calculations and supports both portfolio value tracking and specific data tracking.
 * 
 * Parameters:
 *   - data: Reference asset to track (nullptr for portfolio value)
 *   - firstopen: Use opening price for first calculation (default: true)
 *   - fund: Fund mode behavior (nullptr for disabled)
 * 
 * Features:
 *   - Rolling window management using deque
 *   - Logarithmic return calculation with error handling
 *   - Support for both portfolio and data-specific tracking
 *   - Fund mode compatibility
 *   - Timeframe-aware calculations
 * 
 * get_analysis():
 *   - Returns OrderedDict with datetime->log_return mappings
 */
class LogReturnsRolling : public TimeFrameAnalyzerBase {
public:
    // Parameters structure matching Python implementation
    struct Params : public TimeFrameAnalyzerBase::Params {
        std::shared_ptr<DataSeries> data;  // Reference asset to track
        bool firstopen;                       // Use opening price for first calculation
        std::shared_ptr<DataSeries> fund;  // Fund mode behavior
        
        Params() : TimeFrameAnalyzerBase::Params(), data(nullptr), firstopen(true), fund(nullptr) {}
    };
    
    LogReturnsRolling();
    LogReturnsRolling(const Params& params);
    virtual ~LogReturnsRolling() = default;
    
    // Override analyzer methods
    void start() override;
    void next() override;
    AnalysisResult get_analysis() const override;
    
    // Override timeframe methods
    void on_dt_over() override;
    
    // Override notification methods
    void notify_fund(double cash, double value, double fundvalue, double shares) override;
    
    // Convenience methods
    const OrderedDict<std::chrono::system_clock::time_point, double>& get_rets() const {
        return rets;
    }
    
    // Get rolling window statistics
    double get_current_value() const { return _value; }
    size_t get_window_size() const { return _values.size(); }
    double get_reference_value() const { return _values.empty() ? 0.0 : _values.front(); }
    
    // Statistics helpers
    std::vector<double> get_returns_history() const;
    double get_mean_return() const;
    double get_std_return() const;
    double get_sharpe_ratio() const;
    
    // Export functionality
    void export_to_csv(const std::string& filename) const;
    
protected:
    Params p;  // Parameters
    
    // Rolling window data storage
    std::deque<double> _values;  // Rolling window of historical values
    
    // Current state
    double _value = 0.0;         // Current period's value
    double _lastvalue = 0.0;     // Previous period's value for reference
    
    // Results storage
    OrderedDict<std::chrono::system_clock::time_point, double> rets;
    
    // Fund mode detection
    bool _fundmode = false;
    
private:
    // Value calculation methods
    double get_current_portfolio_value() const;
    double get_current_data_value() const;
    double get_current_fund_value() const;
    
    // Log return calculation
    double calculate_log_return(double current_value, double reference_value) const;
    
    // Error handling for log calculations
    bool is_valid_for_log(double value) const;
    
    // Rolling window management
    void update_rolling_window(double new_value);
    void initialize_rolling_window();
    
    // Helper methods
    std::string format_datetime(const std::chrono::system_clock::time_point& dt) const;
    
    // Statistics calculation helpers
    void calculate_statistics() const;
    
    // Cached statistics (mutable for lazy evaluation)
    mutable bool stats_cached = false;
    mutable double cached_mean_return = 0.0;
    mutable double cached_std_return = 0.0;
    mutable double cached_sharpe_ratio = 0.0;
    
    void invalidate_stats_cache();
    void update_stats_cache() const;
};

} // namespace analyzers
} // namespace backtrader

// Register the analyzer
REGISTER_ANALYZER(backtrader::analyzers::LogReturnsRolling);