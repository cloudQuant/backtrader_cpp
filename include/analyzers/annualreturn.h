#pragma once

#include "../analyzer.h"
#include <map>
#include <vector>
#include <chrono>

namespace backtrader {
namespace analyzers {

/**
 * AnnualReturn analyzer - calculates annual returns by looking at beginning and end of year
 * 
 * This analyzer calculates the AnnualReturns by looking at the beginning
 * and end of the year
 * 
 * Member Attributes:
 *   - rets: list of calculated annual returns  
 *   - ret: ordered map (key: year) of annual returns
 * 
 * get_analysis():
 *   - Returns an ordered map of annual returns (key: year)
 */
class AnnualReturn : public Analyzer {
public:
    AnnualReturn();
    virtual ~AnnualReturn() = default;
    
    // Override analyzer methods
    void stop() override;
    AnalysisResult get_analysis() const override;
    
protected:
    std::vector<double> rets;  // List of calculated annual returns
    OrderedDict<int, double> ret;  // Ordered map (year -> return)
    
private:
    // Helper method to get broker value at specific index
    double get_broker_value(int index) const;
    
    // Helper method to get date at specific index  
    std::chrono::system_clock::time_point get_datetime(int index) const;
    
    // Convert time_point to year
    int get_year(std::chrono::system_clock::time_point tp) const;
};

/**
 * MyAnnualReturn - Alternative implementation using modern C++ and pandas-like logic
 * 
 * This analyzer calculates the AnnualReturns using more efficient algorithms
 * inspired by the pandas implementation from the Python version.
 * 
 * Note: This is the "pandas-style" implementation mentioned in Python version
 */
class MyAnnualReturn : public Analyzer {
public:
    MyAnnualReturn();
    virtual ~MyAnnualReturn() = default;
    
    // Override analyzer methods
    void stop() override;
    AnalysisResult get_analysis() const override;
    
protected:
    OrderedDict<int, double> ret;  // Ordered map (year -> return)
    
private:
    // Data structure to hold datetime-value pairs
    struct DateValuePair {
        std::chrono::system_clock::time_point datetime;
        double value;
        double pre_value;
        int year;
    };
    
    // Helper methods
    std::vector<DateValuePair> get_data_series() const;
    void calculate_annual_returns(const std::vector<DateValuePair>& data);
    int get_year(std::chrono::system_clock::time_point tp) const;
};

} // namespace analyzers
} // namespace backtrader

// Register the analyzers
REGISTER_ANALYZER(backtrader::analyzers::AnnualReturn);
REGISTER_ANALYZER(backtrader::analyzers::MyAnnualReturn);