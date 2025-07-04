#include "../../include/utils/date.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace backtrader {
namespace utils {

// Static member initialization
const std::map<std::string, std::chrono::hours> DateUtils::timezone_offsets_ = DateUtils::init_timezone_offsets();

std::chrono::system_clock::time_point DateUtils::str_to_datetime(const std::string& date_str, const std::string& format) {
    std::tm tm = {};
    std::istringstream ss(date_str);
    
    if (format == "%Y-%m-%d") {
        ss >> std::get_time(&tm, "%Y-%m-%d");
    } else if (format == "%Y-%m-%d %H:%M:%S") {
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    } else if (format == "%d/%m/%Y") {
        ss >> std::get_time(&tm, "%d/%m/%Y");
    } else if (format == "%m/%d/%Y") {
        ss >> std::get_time(&tm, "%m/%d/%Y");
    } else {
        // Default format
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    }
    
    if (ss.fail()) {
        throw std::invalid_argument("Failed to parse date string: " + date_str);
    }
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string DateUtils::datetime_to_str(const std::chrono::system_clock::time_point& tp, const std::string& format) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream ss;
    
    if (format == "%Y-%m-%d") {
        ss << std::put_time(&tm, "%Y-%m-%d");
    } else if (format == "%Y-%m-%d %H:%M:%S") {
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    } else if (format == "%d/%m/%Y") {
        ss << std::put_time(&tm, "%d/%m/%Y");
    } else if (format == "%m/%d/%Y") {
        ss << std::put_time(&tm, "%m/%d/%Y");
    } else {
        // Default format
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    }
    
    return ss.str();
}

double DateUtils::datetime_to_timestamp(const std::chrono::system_clock::time_point& tp) {
    auto duration = tp.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration - seconds);
    
    return static_cast<double>(seconds.count()) + static_cast<double>(microseconds.count()) / 1000000.0;
}

std::chrono::system_clock::time_point DateUtils::timestamp_to_datetime(double timestamp) {
    auto seconds = static_cast<long long>(timestamp);
    auto microseconds = static_cast<long long>((timestamp - seconds) * 1000000);
    
    auto duration = std::chrono::seconds(seconds) + std::chrono::microseconds(microseconds);
    return std::chrono::system_clock::time_point(duration);
}

std::chrono::system_clock::time_point DateUtils::now() {
    return std::chrono::system_clock::now();
}

std::chrono::system_clock::time_point DateUtils::today() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    // Set time to midnight
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::chrono::system_clock::time_point DateUtils::add_days(const std::chrono::system_clock::time_point& tp, int days) {
    return tp + std::chrono::hours(24 * days);
}

std::chrono::system_clock::time_point DateUtils::add_hours(const std::chrono::system_clock::time_point& tp, int hours) {
    return tp + std::chrono::hours(hours);
}

std::chrono::system_clock::time_point DateUtils::add_minutes(const std::chrono::system_clock::time_point& tp, int minutes) {
    return tp + std::chrono::minutes(minutes);
}

std::chrono::system_clock::time_point DateUtils::add_seconds(const std::chrono::system_clock::time_point& tp, int seconds) {
    return tp + std::chrono::seconds(seconds);
}

int DateUtils::get_year(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return tm.tm_year + 1900;
}

int DateUtils::get_month(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return tm.tm_mon + 1; // tm_mon is 0-based
}

int DateUtils::get_day(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return tm.tm_mday;
}

int DateUtils::get_hour(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return tm.tm_hour;
}

int DateUtils::get_minute(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return tm.tm_min;
}

int DateUtils::get_second(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return tm.tm_sec;
}

int DateUtils::get_weekday(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    return tm.tm_wday; // 0 = Sunday, 1 = Monday, etc.
}

bool DateUtils::is_weekend(const std::chrono::system_clock::time_point& tp) {
    int weekday = get_weekday(tp);
    return weekday == 0 || weekday == 6; // Sunday or Saturday
}

bool DateUtils::is_business_day(const std::chrono::system_clock::time_point& tp) {
    return !is_weekend(tp);
}

std::chrono::system_clock::time_point DateUtils::convert_timezone(
    const std::chrono::system_clock::time_point& tp,
    const std::string& from_tz,
    const std::string& to_tz) {
    
    auto from_offset = get_timezone_offset(from_tz);
    auto to_offset = get_timezone_offset(to_tz);
    
    // Convert: remove from_tz offset, add to_tz offset
    return tp - from_offset + to_offset;
}

std::chrono::system_clock::time_point DateUtils::to_utc(
    const std::chrono::system_clock::time_point& tp,
    const std::string& from_tz) {
    return convert_timezone(tp, from_tz, "UTC");
}

std::chrono::system_clock::time_point DateUtils::from_utc(
    const std::chrono::system_clock::time_point& tp,
    const std::string& to_tz) {
    return convert_timezone(tp, "UTC", to_tz);
}

std::chrono::hours DateUtils::get_timezone_offset(const std::string& timezone) {
    auto it = timezone_offsets_.find(timezone);
    if (it != timezone_offsets_.end()) {
        return it->second;
    }
    
    // Default to UTC if timezone not found
    return std::chrono::hours(0);
}

std::map<std::string, std::chrono::hours> DateUtils::init_timezone_offsets() {
    std::map<std::string, std::chrono::hours> offsets;
    
    // Major timezone offsets (simplified, not accounting for DST)
    offsets["UTC"] = std::chrono::hours(0);
    offsets["GMT"] = std::chrono::hours(0);
    offsets["EST"] = std::chrono::hours(-5);
    offsets["CST"] = std::chrono::hours(-6);
    offsets["MST"] = std::chrono::hours(-7);
    offsets["PST"] = std::chrono::hours(-8);
    offsets["CET"] = std::chrono::hours(1);
    offsets["JST"] = std::chrono::hours(9);
    offsets["AEST"] = std::chrono::hours(10);
    offsets["HST"] = std::chrono::hours(-10);
    
    return offsets;
}

bool DateUtils::is_valid_date(int year, int month, int day) {
    if (month < 1 || month > 12) return false;
    if (day < 1) return false;
    
    // Days in month
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Check for leap year
    if (month == 2 && is_leap_year(year)) {
        return day <= 29;
    }
    
    return day <= days_in_month[month - 1];
}

bool DateUtils::is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

std::chrono::system_clock::time_point DateUtils::get_market_open(
    const std::chrono::system_clock::time_point& date,
    int hour, int minute) {
    
    auto time_t = std::chrono::system_clock::to_time_t(date);
    auto tm = *std::localtime(&time_t);
    
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = 0;
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::chrono::system_clock::time_point DateUtils::get_market_close(
    const std::chrono::system_clock::time_point& date,
    int hour, int minute) {
    
    return get_market_open(date, hour, minute);
}

bool DateUtils::is_market_hours(
    const std::chrono::system_clock::time_point& tp,
    int open_hour, int open_minute,
    int close_hour, int close_minute) {
    
    int current_hour = get_hour(tp);
    int current_minute = get_minute(tp);
    
    int current_total_minutes = current_hour * 60 + current_minute;
    int open_total_minutes = open_hour * 60 + open_minute;
    int close_total_minutes = close_hour * 60 + close_minute;
    
    return current_total_minutes >= open_total_minutes && 
           current_total_minutes <= close_total_minutes;
}

} // namespace utils
} // namespace backtrader