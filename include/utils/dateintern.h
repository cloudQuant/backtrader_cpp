#pragma once

#include <chrono>
#include <string>
#include <array>
#include <ctime>

namespace backtrader {
namespace utils {

/**
 * DateIntern - Internal date/time utilities for backtrader
 * 
 * Provides internal date/time handling functions used throughout backtrader.
 * Includes timezone conversion, date arithmetic, and format parsing.
 */
class DateIntern {
public:
    // Time zones
    enum class TimeZone {
        UTC,
        LOCAL,
        EASTERN,    // US Eastern
        CENTRAL,    // US Central  
        MOUNTAIN,   // US Mountain
        PACIFIC,    // US Pacific
        LONDON,     // GMT/BST
        TOKYO,      // JST
        SYDNEY      // AEST/AEDT
    };
    
    // Date formats
    enum class DateFormat {
        ISO8601,        // 2023-12-25T10:30:00Z
        US_FORMAT,      // 12/25/2023 10:30:00
        EUROPEAN,       // 25/12/2023 10:30:00
        TIMESTAMP,      // Unix timestamp
        EXCEL_SERIAL    // Excel serial date
    };
    
    // Constants
    static constexpr int DAYS_PER_WEEK = 7;
    static constexpr int HOURS_PER_DAY = 24;
    static constexpr int MINUTES_PER_HOUR = 60;
    static constexpr int SECONDS_PER_MINUTE = 60;
    static constexpr int MILLISECONDS_PER_SECOND = 1000;
    
    // Excel epoch start (January 1, 1900)
    static constexpr int EXCEL_EPOCH_OFFSET = 25569; // Days from Unix epoch to Excel epoch
    
    // Date conversion methods
    static std::chrono::system_clock::time_point from_timestamp(double timestamp);
    static std::chrono::system_clock::time_point from_excel_serial(double serial_date);
    static std::chrono::system_clock::time_point from_string(const std::string& date_str, 
                                                           DateFormat format = DateFormat::ISO8601);
    
    static double to_timestamp(const std::chrono::system_clock::time_point& time_point);
    static double to_excel_serial(const std::chrono::system_clock::time_point& time_point);
    static std::string to_string(const std::chrono::system_clock::time_point& time_point,
                                DateFormat format = DateFormat::ISO8601);
    
    // Time zone conversion
    static std::chrono::system_clock::time_point convert_timezone(
        const std::chrono::system_clock::time_point& time_point,
        TimeZone from_tz, TimeZone to_tz);
    
    // Date arithmetic
    static std::chrono::system_clock::time_point add_days(
        const std::chrono::system_clock::time_point& time_point, int days);
    static std::chrono::system_clock::time_point add_hours(
        const std::chrono::system_clock::time_point& time_point, int hours);
    static std::chrono::system_clock::time_point add_minutes(
        const std::chrono::system_clock::time_point& time_point, int minutes);
    
    // Date queries
    static int get_year(const std::chrono::system_clock::time_point& time_point);
    static int get_month(const std::chrono::system_clock::time_point& time_point);
    static int get_day(const std::chrono::system_clock::time_point& time_point);
    static int get_hour(const std::chrono::system_clock::time_point& time_point);
    static int get_minute(const std::chrono::system_clock::time_point& time_point);
    static int get_second(const std::chrono::system_clock::time_point& time_point);
    static int get_weekday(const std::chrono::system_clock::time_point& time_point); // 0=Sunday
    
    // Trading day utilities
    static bool is_trading_day(const std::chrono::system_clock::time_point& time_point);
    static bool is_weekend(const std::chrono::system_clock::time_point& time_point);
    static std::chrono::system_clock::time_point next_trading_day(
        const std::chrono::system_clock::time_point& time_point);
    static std::chrono::system_clock::time_point previous_trading_day(
        const std::chrono::system_clock::time_point& time_point);
    
    // Session utilities  
    static std::chrono::system_clock::time_point get_session_start(
        const std::chrono::system_clock::time_point& time_point,
        int start_hour = 9, int start_minute = 0);
    static std::chrono::system_clock::time_point get_session_end(
        const std::chrono::system_clock::time_point& time_point,
        int end_hour = 17, int end_minute = 0);
    
    // Date validation
    static bool is_valid_date(int year, int month, int day);
    static bool is_valid_time(int hour, int minute, int second = 0);
    static bool is_leap_year(int year);
    
    // Holiday detection (basic)
    static bool is_us_holiday(const std::chrono::system_clock::time_point& time_point);
    static std::vector<std::chrono::system_clock::time_point> get_us_holidays(int year);

private:
    // Month names
    static const std::array<std::string, 12> MONTH_NAMES;
    static const std::array<std::string, 12> MONTH_ABBREV;
    
    // Day names
    static const std::array<std::string, 7> DAY_NAMES;
    static const std::array<std::string, 7> DAY_ABBREV;
    
    // Timezone offset helpers
    static std::chrono::hours get_timezone_offset(TimeZone tz);
    static bool is_dst(const std::chrono::system_clock::time_point& time_point, TimeZone tz);
    
    // Date parsing helpers
    static std::chrono::system_clock::time_point parse_iso8601(const std::string& date_str);
    static std::chrono::system_clock::time_point parse_us_format(const std::string& date_str);
    static std::chrono::system_clock::time_point parse_european_format(const std::string& date_str);
    
    // Date formatting helpers
    static std::string format_iso8601(const std::chrono::system_clock::time_point& time_point);
    static std::string format_us(const std::chrono::system_clock::time_point& time_point);
    static std::string format_european(const std::chrono::system_clock::time_point& time_point);
    
    // Utility functions
    static std::tm to_tm(const std::chrono::system_clock::time_point& time_point);
    static std::chrono::system_clock::time_point from_tm(const std::tm& tm_time);
    static int days_in_month(int year, int month);
};

} // namespace utils
} // namespace backtrader