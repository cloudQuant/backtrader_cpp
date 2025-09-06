#pragma once

#include "../analyzer.h"
#include <vector>
#include <map>
#include <cmath>

namespace backtrader {

// Returns Analyzer - calculates total, average, compound and annualized returns
class Returns : public TimeFrameAnalyzerBase {
public:
    struct Params {
        int timeframe = 0;          // 0 means use data timeframe
        int compression = 0;        // 0 means use data compression
        double tann = 0.0;          // Annualization periods (252 for days, 52 for weeks, etc.)
        bool fundmode = false;      // Fund mode calculation
        double fund_startval = 100.0; // Starting fund value
    } params;
    
    struct Results {
        double rtot = 0.0;          // Total return
        double ravg = 0.0;          // Average return
        double rnorm = 0.0;         // Normalized return
        double rnorm100 = 0.0;      // Normalized return * 100
    };
    
    Returns();
    virtual ~Returns() = default;
    
    // Analyzer interface
    void start() override;
    void next() override;
    void stop() override;
    
    // Get results
    Results get_results() const { return results_; }
    
    // Get analysis data
    std::map<std::string, double> get_analysis() override;
    
private:
    Results results_;
    
    // Calculation data
    std::vector<double> returns_;
    double last_value_;
    bool first_value_;
    int period_count_;
    
    // Helper methods
    double calculate_log_return(double current_value, double previous_value);
    void calculate_final_results();
    double get_annualization_factor();
};

} // namespace backtrader