#pragma once

#include "../analyzer.h"
#include "../timeframe.h"
#include <map>
#include <string>
#include <vector>

namespace backtrader {

// TimeReturn Analyzer - calculates returns by looking at beginning and end of timeframe
class TimeReturn : public TimeFrameAnalyzerBase {
public:
    struct Params {
        int timeframe = 0;              // 0 means use data timeframe
        int compression = 0;            // 0 means use data compression
        bool firstopen = true;          // Use opening price for first calculation
        bool fund = false;              // Use fund mode
        bool auto_fund = true;          // Auto-detect fund mode
        // data parameter would be a pointer to specific data feed (not implemented in this version)
    } params;
    
    TimeReturn();
    explicit TimeReturn(const std::string& name);
    TimeReturn(const std::string& name, TimeFrame timeframe);
    virtual ~TimeReturn() = default;
    
    // Analyzer interface
    void start() override;
    void next() override;
    void notify_fund(double cash, double value, double fundvalue, double shares) override;
    
    // TimeFrameAnalyzerBase interface
    void on_dt_over() override;
    
    // Get analysis data  
    AnalysisResult get_analysis() const override;
    
    // Get returns vector (for child analyzers)
    std::vector<double> get_returns() const;
    
private:
    // Calculation data
    double value_start_;
    double last_value_;
    double current_value_;
    bool fundmode_;
    
    // Return storage
    std::map<std::string, double> returns_;
    
    // Helper methods
    std::string get_current_date_key() const;
};

} // namespace backtrader