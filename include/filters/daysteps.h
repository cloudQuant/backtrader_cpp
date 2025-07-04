#pragma once

#include "../feed.h"
#include <memory>
#include <chrono>
#include <vector>

namespace backtrader {
namespace filters {

/**
 * DaySteps - Day steps filter
 * 
 * Filters bars to only include data at specific step intervals within a day.
 * Useful for creating regular sampling intervals from irregular data.
 * 
 * Params:
 *   - days: Number of days to step (default: 1)
 *   - step_size: Step size in minutes within a day
 *   - start_time: Start time for stepping (format: "HH:MM")
 *   - end_time: End time for stepping (format: "HH:MM")
 */
class DaySteps {
public:
    // Parameters structure
    struct Params {
        int days = 1;
        int step_size = 60;  // minutes
        std::string start_time = "09:00";
        std::string end_time = "17:00";
    };

    DaySteps(std::shared_ptr<AbstractDataBase> data, const Params& params = Params{});
    virtual ~DaySteps() = default;

    // Filter interface
    bool operator()(std::shared_ptr<AbstractDataBase> data);

private:
    // Parameters
    Params params_;
    
    // Time boundaries
    std::chrono::minutes start_minutes_;
    std::chrono::minutes end_minutes_;
    std::chrono::minutes step_minutes_;
    
    // State tracking
    std::chrono::system_clock::time_point last_day_;
    std::chrono::minutes last_step_time_;
    
    // Internal methods
    std::chrono::minutes parse_time(const std::string& time_str) const;
    std::chrono::minutes get_minutes_from_midnight(
        const std::chrono::system_clock::time_point& datetime) const;
    
    std::chrono::system_clock::time_point get_date_only(
        const std::chrono::system_clock::time_point& datetime) const;
    
    bool is_valid_step_time(const std::chrono::system_clock::time_point& datetime) const;
    bool should_include_bar(const std::chrono::system_clock::time_point& datetime);
};

} // namespace filters
} // namespace backtrader