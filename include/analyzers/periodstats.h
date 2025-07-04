#pragma once

#include "../analyzer.h"
#include "../timeframe.h"
#include <chrono>
#include <map>
#include <vector>

namespace backtrader {
namespace analyzers {

/**
 * PeriodStats - Period statistics analyzer
 * 
 * Analyzes performance statistics over specific time periods (daily, weekly, monthly, yearly).
 * Provides detailed breakdowns of returns and performance metrics by period.
 */
class PeriodStats : public Analyzer {
public:
    // Period statistics structure
    struct PeriodData {
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        double start_value;
        double end_value;
        double return_value;
        double return_pct;
        double high_value;
        double low_value;
        double volatility;
        int trading_days;
    };

    // Parameters structure
    struct Params {
        TimeFrame timeframe = TimeFrame::Months;  // Period timeframe
        int compression = 1;                      // Compression factor
        bool fund = false;                        // Fund mode calculation
        double initial_cash = 100000.0;          // Initial cash for calculations
    };

    PeriodStats(const Params& params = Params{});
    virtual ~PeriodStats() = default;

    // Analyzer interface
    void start() override;
    void next() override;
    void stop() override;
    AnalysisResult get_analysis() override;

    // Period access
    const std::vector<PeriodData>& get_all_periods() const;
    PeriodData get_current_period() const;
    PeriodData get_period(int index) const;
    int get_period_count() const;
    
    // Performance metrics
    double get_best_period_return() const;
    double get_worst_period_return() const;
    double get_average_period_return() const;
    double get_period_volatility() const;
    
    // Period analysis
    int get_positive_periods() const;
    int get_negative_periods() const;
    double get_win_rate() const;
    double get_average_positive_return() const;
    double get_average_negative_return() const;
    
    // Risk metrics
    double get_maximum_drawdown_period() const;
    double get_period_sharpe_ratio() const;
    double get_period_sortino_ratio() const;

private:
    // Parameters
    Params params_;
    
    // Period tracking
    std::vector<PeriodData> periods_;
    PeriodData current_period_;
    bool in_period_ = false;
    
    // Time management
    std::chrono::system_clock::time_point next_period_boundary_;
    
    // Value tracking
    std::vector<double> daily_values_;
    double period_high_ = 0.0;
    double period_low_ = std::numeric_limits<double>::max();
    
    // Statistics
    int positive_periods_ = 0;
    int negative_periods_ = 0;
    double sum_returns_ = 0.0;
    double sum_positive_returns_ = 0.0;
    double sum_negative_returns_ = 0.0;
    double sum_squared_returns_ = 0.0;
    
    // Internal methods
    void check_period_boundary();
    void start_new_period();
    void finalize_current_period();
    void update_period_statistics();
    
    // Time boundary calculations
    std::chrono::system_clock::time_point calculate_next_boundary();
    std::chrono::system_clock::time_point get_period_start(
        const std::chrono::system_clock::time_point& time);
    std::chrono::system_clock::time_point get_period_end(
        const std::chrono::system_clock::time_point& start);
    
    // Period calculations
    void calculate_period_return(PeriodData& period);
    void calculate_period_volatility(PeriodData& period);
    void update_period_extremes();
    
    // Statistical calculations
    double calculate_average_return() const;
    double calculate_return_volatility() const;
    double calculate_sharpe_ratio() const;
    double calculate_sortino_ratio() const;
    
    // Utility methods
    double get_broker_value() const;
    bool is_new_period() const;
    int get_trading_days_in_period() const;
    
    // Period classification
    void classify_period_return(double return_value);
    bool is_trading_day(const std::chrono::system_clock::time_point& date) const;
    
    // Time utilities specific to different timeframes
    std::chrono::system_clock::time_point get_month_start(
        const std::chrono::system_clock::time_point& time) const;
    std::chrono::system_clock::time_point get_year_start(
        const std::chrono::system_clock::time_point& time) const;
    std::chrono::system_clock::time_point get_quarter_start(
        const std::chrono::system_clock::time_point& time) const;
};

} // namespace analyzers
} // namespace backtrader