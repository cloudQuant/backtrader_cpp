#pragma once

#include "../analyzer.h"
#include "timereturn.h"
#include "annualreturn.h"
#include <map>
#include <string>
#include <vector>

namespace backtrader {

// SharpeRatio Analyzer - calculates the Sharpe ratio of a strategy
class SharpeRatio : public Analyzer {
public:
    enum TimeFrame {
        NoTimeFrame = 0,
        Days = 1,
        Weeks = 2,
        Months = 3,
        Years = 4
    };
    
    struct Params {
        TimeFrame timeframe = TimeFrame::Years;  // Trading timeframe
        int compression = 1;                     // Compression factor
        double riskfreerate = 0.01;             // Risk-free rate (annual)
        double factor = -1.0;                   // Conversion factor (-1 means auto)
        bool convertrate = true;                // Convert risk-free rate to timeframe
        bool annualize = false;                 // Annualize the Sharpe ratio
        bool stddev_sample = false;             // Use sample standard deviation
        
        // Legacy parameters
        double daysfactor = -1.0;               // Old naming for factor
        bool legacyannual = false;              // Use AnnualReturn analyzer
        bool fund = false;                      // Use fund mode
        bool auto_fund = true;                  // Auto-detect fund mode
    } params;
    
    SharpeRatio();
    virtual ~SharpeRatio() = default;
    
    // Analyzer interface
    void start() override;
    void stop() override;
    
    // Get analysis data
    std::map<std::string, double> get_analysis() override;
    
    // Get Sharpe ratio result
    double get_sharpe_ratio() const { return sharpe_ratio_; }
    
private:
    // Rate conversion factors
    static const std::map<TimeFrame, double> RATE_FACTORS;
    
    // Child analyzers
    std::unique_ptr<TimeReturn> timereturn_;
    std::unique_ptr<AnnualReturn> annualreturn_;
    
    // Results
    double sharpe_ratio_;
    
    // Helper methods
    double get_conversion_factor() const;
    double calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate);
    std::vector<double> calculate_excess_returns(const std::vector<double>& returns, double risk_free_rate);
    double calculate_average(const std::vector<double>& values);
    double calculate_standard_deviation(const std::vector<double>& values, double mean, bool bessel = false);
};

// SharpeRatioA - Annualized version of SharpeRatio
class SharpeRatioA : public SharpeRatio {
public:
    SharpeRatioA() {
        params.annualize = true;
    }
    virtual ~SharpeRatioA() = default;
};

} // namespace backtrader