#pragma once

#include "../observer.h"
#include "../timeframe.h"
#include <chrono>
#include <map>

namespace backtrader {
namespace observers {

/**
 * TimeReturn - Time-based return observer
 * 
 * Calculates returns over specific time periods (daily, weekly, monthly, yearly).
 * Useful for analyzing performance across different time horizons.
 */
class TimeReturn : public Observer {
public:
    // Parameters structure
    struct Params {
        TimeFrame timeframe = TimeFrame::Days;  // Time period for returns
        int compression = 1;                    // Compression factor
        bool fund = false;                      // Fund mode calculation
        bool firstopen = true;                  // Use first bar for initial value
        double initial_value = 100.0;           // Initial portfolio value (for normalization)
    };

    // Lines
    enum Lines {
        TIMERETURN = 0
    };

    TimeReturn(const Params& params = Params{});
    virtual ~TimeReturn() = default;

    // Observer interface
    void next() override;
    void start() override;
    void stop() override;

    // Return analysis
    double get_current_period_return() const;
    double get_last_period_return() const;
    double get_period_return(int periods_back) const;
    
    // Statistical metrics
    double get_average_return() const;
    double get_best_return() const;
    double get_worst_return() const;
    double get_volatility() const;
    double get_sharpe_ratio(double risk_free_rate = 0.0) const;
    
    // Period analysis
    int get_positive_periods() const;
    int get_negative_periods() const;
    double get_win_rate() const;
    int get_total_periods() const;

private:
    // Parameters
    Params params_;
    
    // Time period tracking
    std::chrono::system_clock::time_point period_start_time_;
    std::chrono::system_clock::time_point current_time_;
    std::chrono::system_clock::time_point next_period_boundary_;
    
    // Value tracking
    double period_start_value_ = 0.0;
    double current_period_value_ = 0.0;
    double initial_portfolio_value_ = 0.0;
    
    // Return history
    std::vector<double> period_returns_;
    std::map<std::chrono::system_clock::time_point, double> return_history_;
    
    // Statistical accumulators
    double sum_returns_ = 0.0;
    double sum_squared_returns_ = 0.0;
    int positive_periods_ = 0;
    int negative_periods_ = 0;
    
    // Internal methods
    void update_current_time();
    void check_period_boundary();
    void start_new_period();
    void finalize_current_period();
    void calculate_period_return();
    
    // Time boundary calculations
    std::chrono::system_clock::time_point calculate_next_boundary();
    std::chrono::system_clock::time_point get_period_start(
        const std::chrono::system_clock::time_point& reference_time);
    std::chrono::system_clock::time_point get_period_end(
        const std::chrono::system_clock::time_point& period_start);
    
    // Time utilities
    bool is_new_period() const;
    std::chrono::duration<double> get_period_duration() const;
    int get_periods_per_year() const;
    
    // Statistical calculations
    double calculate_average_return() const;
    double calculate_volatility() const;
    double calculate_sharpe_ratio(double risk_free_rate) const;
    
    // Period classification
    void classify_period_return(double return_value);
    
    // Utility methods
    double get_broker_value() const;
    double normalize_value(double value) const;
    
    // Time frame specific methods
    std::chrono::system_clock::time_point get_day_start(
        const std::chrono::system_clock::time_point& time) const;
    std::chrono::system_clock::time_point get_week_start(
        const std::chrono::system_clock::time_point& time) const;
    std::chrono::system_clock::time_point get_month_start(
        const std::chrono::system_clock::time_point& time) const;
    std::chrono::system_clock::time_point get_year_start(
        const std::chrono::system_clock::time_point& time) const;
    
    // Validation
    bool is_valid_timeframe() const;
    bool has_sufficient_data() const;
};

} // namespace observers
} // namespace backtrader