#pragma once

#include "../analyzer.h"
#include "drawdown.h"
#include <deque>
#include <map>
#include <string>
#include <cmath>

namespace backtrader {

// Calmar Analyzer - calculates the Calmar ratio (annualized return / max drawdown)
class Calmar : public TimeFrameAnalyzerBase {
public:
    enum TimeFrame {
        NoTimeFrame = 0,
        Days = 1,
        Weeks = 2,
        Months = 3,
        Years = 4
    };
    
    struct Params {
        TimeFrame timeframe = TimeFrame::Months;  // Default timeframe for Calmar
        int compression = 1;                      // Compression factor
        int period = 36;                          // Rolling period (36 months default)
        bool fund = false;                        // Use fund mode
        bool auto_fund = true;                    // Auto-detect fund mode
    } params;
    
    Calmar();
    virtual ~Calmar() = default;
    
    // Analyzer interface
    void start() override;
    void stop() override;
    
    // TimeFrameAnalyzerBase interface
    void on_dt_over() override;
    
    // Get analysis data
    std::map<std::string, double> get_analysis() override;
    
    // Get current Calmar ratio
    double get_calmar_ratio() const { return current_calmar_; }
    
private:
    // Child analyzer for maximum drawdown
    std::unique_ptr<TimeDrawDown> max_drawdown_;
    
    // Rolling values storage
    std::deque<double> values_;
    
    // State tracking
    double max_dd_;
    double current_calmar_;
    bool fundmode_;
    
    // Results storage
    std::map<std::string, double> results_;
    
    // Helper methods
    double calculate_annualized_return();
    std::string get_current_date_key() const;
};

} // namespace backtrader