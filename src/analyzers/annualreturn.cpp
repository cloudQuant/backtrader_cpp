#include "analyzers/annualreturn.h"
#include "strategy.h"
#include "broker.h"
#include "dataseries.h"
#include <algorithm>
#include <ctime>
#include <numeric>

namespace backtrader {
namespace analyzers {

// AnnualReturn implementation
AnnualReturn::AnnualReturn() : Analyzer() {
    // Initialize analysis storage
    rets.clear();
    ret = OrderedDict<int, double>();
}

void AnnualReturn::stop() {
    // Implementation matching Python version logic
    int cur_year = -1;
    double value_start = 0.0;
    double value_end = 0.0;
    
    if (!strategy || !strategy->get_broker()) {
        return;
    }
    
    // Get data length (assuming we have access to strategy data)
    size_t data_length = 0;
    if (!datas.empty() && datas[0]) {
        data_length = datas[0]->size();
    } else {
        return;
    }
    
    // Iterate through data from newest to oldest (matching Python: range(len(self.data) - 1, -1, -1))
    for (int i = static_cast<int>(data_length) - 1; i >= 0; --i) {
        auto dt = get_datetime(i);
        double value_cur = get_broker_value(i);
        int year = get_year(dt);
        
        // Check if we've moved to a new year
        if (year > cur_year) {
            if (cur_year >= 0) {
                // Calculate annual return for the previous year
                double annual_ret = (value_start != 0.0) ? (value_end / value_start) - 1.0 : 0.0;
                rets.push_back(annual_ret);
                ret[cur_year] = annual_ret;
                
                // Use last value as new start
                value_start = value_end;
            } else {
                // First year - set initial value
                value_start = value_cur;
            }
            
            cur_year = year;
        }
        
        // Always update end value
        value_end = value_cur;
    }
    
    // Calculate return for the final year if not already calculated
    if (ret.contains(cur_year) == false && cur_year >= 0) {
        double annual_ret = (value_start != 0.0) ? (value_end / value_start) - 1.0 : 0.0;
        rets.push_back(annual_ret);
        ret[cur_year] = annual_ret;
    }
}

AnalysisResult AnnualReturn::get_analysis() const {
    AnalysisResult result;
    
    // Convert OrderedDict to AnalysisResult format
    std::map<std::string, double> ret_map;
    for (const auto& pair : ret) {
        ret_map[std::to_string(pair.first)] = pair.second;
    }
    
    result["returns"] = ret_map;
    return result;
}

double AnnualReturn::get_broker_value(int index) const {
    if (!strategy || !strategy->get_broker()) {
        return 0.0;
    }
    
    // This would need to be implemented based on broker interface
    // For now, return a placeholder
    return strategy->get_broker()->get_value();
}

std::chrono::system_clock::time_point AnnualReturn::get_datetime(int index) const {
    if (datas.empty() || !datas[0]) {
        return std::chrono::system_clock::now();
    }
    
    // This would need to be implemented based on data interface
    // For now, return current time as placeholder
    return std::chrono::system_clock::now();
}

int AnnualReturn::get_year(std::chrono::system_clock::time_point tp) const {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return tm.tm_year + 1900;
}

// MyAnnualReturn implementation
MyAnnualReturn::MyAnnualReturn() : Analyzer() {
    ret = OrderedDict<int, double>();
}

void MyAnnualReturn::stop() {
    try {
        auto data_series = get_data_series();
        calculate_annual_returns(data_series);
    } catch (const std::exception& e) {
        // Log error or handle gracefully
        // For now, just clear results
        ret = OrderedDict<int, double>();
    }
}

AnalysisResult MyAnnualReturn::get_analysis() const {
    AnalysisResult result;
    
    // Convert OrderedDict to AnalysisResult format
    std::map<std::string, double> ret_map;
    for (const auto& pair : ret) {
        ret_map[std::to_string(pair.first)] = pair.second;
    }
    
    result["returns"] = ret_map;
    return result;
}

std::vector<MyAnnualReturn::DateValuePair> MyAnnualReturn::get_data_series() const {
    std::vector<DateValuePair> result;
    
    if (!strategy || !strategy->get_broker() || datas.empty() || !datas[0]) {
        return result;
    }
    
    size_t data_length = datas[0]->size();
    result.reserve(data_length);
    
    for (size_t i = 0; i < data_length; ++i) {
        DateValuePair pair;
        
        // Get datetime and value (these would need proper implementation)
        pair.datetime = std::chrono::system_clock::now(); // Placeholder
        pair.value = strategy->get_broker()->get_value();  // Placeholder
        pair.year = get_year(pair.datetime);
        
        // Calculate previous value (shift(1) equivalent)
        if (i > 0) {
            pair.pre_value = result[i-1].value;
        } else {
            pair.pre_value = std::numeric_limits<double>::quiet_NaN();
        }
        
        result.push_back(pair);
    }
    
    return result;
}

void MyAnnualReturn::calculate_annual_returns(const std::vector<DateValuePair>& data) {
    if (data.empty()) {
        return;
    }
    
    // Group by year and calculate returns
    std::map<int, std::vector<const DateValuePair*>> year_groups;
    
    for (const auto& pair : data) {
        year_groups[pair.year].push_back(&pair);
    }
    
    // Calculate annual returns for each year
    for (const auto& [year, year_data] : year_groups) {
        if (year_data.empty()) {
            continue;
        }
        
        // Get begin and end values for the year
        double begin_value = year_data[0]->pre_value;
        double end_value = year_data.back()->value;
        
        // Skip if begin_value is NaN or zero
        if (std::isnan(begin_value) || begin_value == 0.0) {
            continue;
        }
        
        double annual_return = (end_value / begin_value) - 1.0;
        ret[year] = annual_return;
    }
}

int MyAnnualReturn::get_year(std::chrono::system_clock::time_point tp) const {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return tm.tm_year + 1900;
}

} // namespace analyzers
} // namespace backtrader