#pragma once

#include <chrono>
#include <string>
#include <ctime>

namespace backtrader {
namespace utils {

/**
 * Date utility functions for time conversion and manipulation
 * 
 * This module provides utilities for converting between different time representations:
 * - num2date, num2dt: Convert numeric timestamps to date/datetime
 * - date2num, time2num: Convert date/time to numeric timestamps  
 * - num2time: Convert numeric timestamps to time
 * - Timezone handling utilities
 */

// Type aliases for clarity
using TimePoint = std::chrono::system_clock::time_point;
using Duration = std::chrono::system_clock::duration;

// Constants
extern const TimePoint TIME_MAX;
extern const TimePoint TIME_MIN;
extern const std::string UTC_TIMEZONE;

// Core conversion functions
/**
 * Convert numeric timestamp to date
 */
std::tm num2date(double num);

/**
 * Convert numeric timestamp to datetime (time_point)
 */
TimePoint num2dt(double num);

/**
 * Convert date to numeric timestamp
 */
double date2num(const std::tm& date);

/**
 * Convert time to numeric timestamp  
 */
double time2num(const std::tm& time);

/**
 * Convert numeric timestamp to time
 */
std::tm num2time(double num);

/**
 * Convert string to datetime
 */
TimePoint str2datetime(const std::string& date_str, const std::string& format = "%Y-%m-%d %H:%M:%S");

/**
 * Convert datetime to string
 */
std::string datetime2str(const TimePoint& dt, const std::string& format = "%Y-%m-%d %H:%M:%S");

/**
 * Convert timestamp to datetime
 */
TimePoint timestamp2datetime(int64_t timestamp, bool milliseconds = false);

/**
 * Convert datetime to timestamp
 */
int64_t datetime2timestamp(const TimePoint& dt, bool milliseconds = false);

/**
 * Get last timestamp for a given timeframe
 */
TimePoint get_last_timeframe_timestamp(const TimePoint& dt, int timeframe);

/**
 * Parse timezone string
 */
class TimeZone {
public:
    TimeZone(const std::string& tz_name = "UTC");
    
    TimePoint localize(const TimePoint& dt) const;
    TimePoint to_utc(const TimePoint& dt) const;
    std::string get_name() const { return name_; }
    
private:
    std::string name_;
    int offset_hours_ = 0;
};

/**
 * Timezone parsing utility
 */
TimeZone tzparse(const std::string& tz_str);

/**
 * Local timezone utility
 */
class TZLocal {
public:
    static TimeZone get();
    static TimePoint now();
};

/**
 * Timezone localizer utility  
 */
class Localizer {
public:
    Localizer(const TimeZone& tz);
    
    TimePoint operator()(const TimePoint& dt) const;
    
private:
    TimeZone tz_;
};

} // namespace utils
} // namespace backtrader