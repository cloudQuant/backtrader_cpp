#pragma once

#include "../observer.h"
#include <algorithm>
#include <limits>

namespace backtrader {
namespace observers {

/**
 * DrawDown - Drawdown observer
 * 
 * Tracks the portfolio drawdown over time.
 * Calculates both absolute and percentage drawdowns from peak values.
 */
class DrawDown : public Observer {
public:
    // Parameters structure  
    struct Params {
        bool fund = false;  // Fund mode (use different calculation)
    };

    // Lines
    enum Lines {
        DRAWDOWN = 0,
        MAX_DRAWDOWN = 1
    };

    DrawDown(const Params& params = Params{});
    virtual ~DrawDown() = default;

    // Observer interface
    void next() override;
    void start() override;
    void stop() override;

    // Drawdown metrics
    double get_current_drawdown() const;
    double get_max_drawdown() const;
    double get_current_drawdown_pct() const;
    double get_max_drawdown_pct() const;
    
    // Peak tracking
    double get_current_peak() const;
    double get_peak_value() const;
    int get_drawdown_length() const;      // Current drawdown duration
    int get_max_drawdown_length() const;  // Longest drawdown duration
    
    // Recovery tracking
    bool is_in_drawdown() const;
    double get_recovery_factor() const;
    int get_days_to_recovery() const;

private:
    // Parameters
    Params params_;
    
    // Peak and drawdown tracking
    double peak_value_ = 0.0;
    double current_value_ = 0.0;
    double max_drawdown_value_ = 0.0;
    double max_drawdown_pct_ = 0.0;
    
    // Duration tracking
    int drawdown_start_index_ = -1;
    int current_drawdown_length_ = 0;
    int max_drawdown_length_ = 0;
    int max_dd_start_index_ = -1;
    int max_dd_end_index_ = -1;
    
    // Recovery tracking
    bool in_drawdown_ = false;
    double drawdown_start_value_ = 0.0;
    
    // Historical tracking
    std::vector<double> portfolio_values_;
    std::vector<double> peak_values_;
    std::vector<double> drawdown_values_;
    std::vector<double> drawdown_pcts_;
    
    // Internal methods
    void update_portfolio_value();
    void update_peak_value();
    void calculate_drawdowns();
    void update_drawdown_duration();
    void check_recovery();
    
    // Calculation methods
    double calculate_absolute_drawdown() const;
    double calculate_percentage_drawdown() const;
    
    // Utility methods
    double get_broker_value() const;
    bool is_new_peak() const;
    void reset_drawdown_tracking();
    void update_max_drawdown_metrics(double current_dd, double current_dd_pct);
};

} // namespace observers
} // namespace backtrader