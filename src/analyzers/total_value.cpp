#include "../../include/analyzers/total_value.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <stdexcept>

namespace backtrader {
namespace analyzers {

TotalValue::TotalValue() {
    // Initialize base class
    Analyzer::Analyzer();
    
    // Clear any existing data
    rets.clear();
    invalidate_stats_cache();
}

void TotalValue::start() {
    // Call parent start
    Analyzer::start();
    
    // Initialize the returns dictionary
    rets.clear();
    invalidate_stats_cache();
}

void TotalValue::next() {
    // Call parent next
    Analyzer::next();
    
    // Get current datetime from the first data feed
    auto current_dt = get_current_datetime();
    
    // Get current broker value
    double current_value = get_current_broker_value();
    
    // Store the datetime-value pair
    rets[current_dt] = current_value;
    
    // Invalidate cached statistics
    invalidate_stats_cache();
}

AnalysisResult TotalValue::get_analysis() const {
    AnalysisResult result;
    
    // Convert OrderedDict to AnalysisResult format
    // Since AnalysisResult uses string keys, we need to convert datetimes to strings
    std::map<std::string, double> datetime_value_map;
    
    for (const auto& pair : rets) {
        std::string datetime_str = format_datetime(pair.first);
        datetime_value_map[datetime_str] = pair.second;
    }
    
    // Store as a map in the result
    result["values"] = datetime_value_map;
    
    // Add summary statistics
    if (!rets.empty()) {
        result["initial_value"] = get_initial_value();
        result["final_value"] = get_final_value();
        result["total_return"] = get_total_return();
        result["max_value"] = get_max_value();
        result["min_value"] = get_min_value();
        result["count"] = static_cast<int>(rets.size());
    }
    
    return result;
}

double TotalValue::get_value_at(const std::chrono::system_clock::time_point& dt) const {
    if (rets.contains(dt)) {
        return rets.at(dt);
    }
    throw std::out_of_range("Datetime not found in total value history");
}

std::vector<double> TotalValue::get_value_history() const {
    std::vector<double> values;
    values.reserve(rets.size());
    
    for (const auto& pair : rets) {
        values.push_back(pair.second);
    }
    
    return values;
}

std::vector<std::chrono::system_clock::time_point> TotalValue::get_datetime_history() const {
    std::vector<std::chrono::system_clock::time_point> datetimes;
    datetimes.reserve(rets.size());
    
    for (const auto& pair : rets) {
        datetimes.push_back(pair.first);
    }
    
    return datetimes;
}

double TotalValue::get_initial_value() const {
    if (rets.empty()) {
        return 0.0;
    }
    
    update_stats_cache();
    return cached_initial_value;
}

double TotalValue::get_final_value() const {
    if (rets.empty()) {
        return 0.0;
    }
    
    update_stats_cache();
    return cached_final_value;
}

double TotalValue::get_total_return() const {
    if (rets.empty()) {
        return 0.0;
    }
    
    double initial = get_initial_value();
    double final = get_final_value();
    
    if (initial == 0.0) {
        return 0.0;
    }
    
    return (final - initial) / initial;
}

double TotalValue::get_max_value() const {
    if (rets.empty()) {
        return 0.0;
    }
    
    update_stats_cache();
    return cached_max_value;
}

double TotalValue::get_min_value() const {
    if (rets.empty()) {
        return 0.0;
    }
    
    update_stats_cache();
    return cached_min_value;
}

void TotalValue::export_to_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    // Write header
    file << "datetime,value\n";
    
    // Write data
    for (const auto& pair : rets) {
        file << format_datetime(pair.first) << "," << std::fixed << std::setprecision(6) << pair.second << "\n";
    }
    
    file.close();
}

double TotalValue::get_current_broker_value() const {
    // Access broker through strategy
    if (!strategy) {
        throw std::runtime_error("Strategy not set - cannot access broker");
    }
    
    // In a real implementation, this would access strategy->broker->getvalue()
    // For now, we'll use a placeholder implementation
    // TODO: Implement actual broker access
    
    // Placeholder: return a dummy value
    // In the real implementation, this would be:
    // return strategy->broker->getvalue();
    
    // For now, simulate some portfolio value
    static double dummy_value = 100000.0;
    static bool initialized = false;
    
    if (!initialized) {
        // Initialize with some base value
        dummy_value = 100000.0;
        initialized = true;
    }
    
    // Add some simulated growth/volatility
    dummy_value += (rand() % 2001 - 1000) * 0.01; // Random change between -10 and +10
    
    return dummy_value;
}

std::chrono::system_clock::time_point TotalValue::get_current_datetime() const {
    // Access datetime from the first data feed
    if (datas.empty() || !datas[0]) {
        // If no data available, use current time
        return std::chrono::system_clock::now();
    }
    
    // In a real implementation, this would access datas[0]->datetime.datetime()
    // For now, we'll use a placeholder implementation
    // TODO: Implement actual data datetime access
    
    // Placeholder: return current time
    // In the real implementation, this would be:
    // return datas[0]->datetime.datetime();
    
    return std::chrono::system_clock::now();
}

std::string TotalValue::format_datetime(const std::chrono::system_clock::time_point& dt) const {
    // Convert time_point to time_t
    std::time_t time_t = std::chrono::system_clock::to_time_t(dt);
    
    // Convert to tm struct
    std::tm* tm_ptr = std::localtime(&time_t);
    
    // Format as ISO string
    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    
    return oss.str();
}

void TotalValue::update_stats_cache() const {
    if (stats_cached || rets.empty()) {
        return;
    }
    
    // Find min and max values
    auto values = get_value_history();
    auto minmax = std::minmax_element(values.begin(), values.end());
    
    cached_min_value = *minmax.first;
    cached_max_value = *minmax.second;
    
    // Get initial and final values
    cached_initial_value = values.front();
    cached_final_value = values.back();
    
    stats_cached = true;
}

void TotalValue::invalidate_stats_cache() {
    stats_cached = false;
}

} // namespace analyzers
} // namespace backtrader