#include "../../include/utils/dateintern.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <ctime>

namespace backtrader {
namespace utils {

// DateIntern implementation
DateIntern::DateIntern(int year, int month, int day) 
    : year_(year), month_(month), day_(day) {
    if (!is_valid_date(year, month, day)) {
        throw std::invalid_argument("Invalid date: " + std::to_string(year) + "-" + 
                                   std::to_string(month) + "-" + std::to_string(day));
    }
}

DateIntern::DateIntern(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    
    year_ = tm.tm_year + 1900;
    month_ = tm.tm_mon + 1;
    day_ = tm.tm_mday;
}

DateIntern::DateIntern(const std::string& date_str, const std::string& format) {
    parse_from_string(date_str, format);
}

DateIntern DateIntern::today() {
    auto now = std::chrono::system_clock::now();
    return DateIntern(now);
}

DateIntern DateIntern::from_timestamp(time_t timestamp) {
    auto tp = std::chrono::system_clock::from_time_t(timestamp);
    return DateIntern(tp);
}

DateIntern DateIntern::from_ordinal(int ordinal) {
    // Convert ordinal day number to date
    // Using epoch as reference point (January 1, 1970 = ordinal 719163)
    const int epoch_ordinal = 719163;
    int days_since_epoch = ordinal - epoch_ordinal;
    
    auto epoch = std::chrono::system_clock::from_time_t(0);
    auto target_time = epoch + std::chrono::hours(24 * days_since_epoch);
    
    return DateIntern(target_time);
}

int DateIntern::year() const {
    return year_;
}

int DateIntern::month() const {
    return month_;
}

int DateIntern::day() const {
    return day_;
}

void DateIntern::set_year(int year) {
    if (!is_valid_date(year, month_, day_)) {
        throw std::invalid_argument("Invalid year: " + std::to_string(year));
    }
    year_ = year;
}

void DateIntern::set_month(int month) {
    if (!is_valid_date(year_, month, day_)) {
        throw std::invalid_argument("Invalid month: " + std::to_string(month));
    }
    month_ = month;
}

void DateIntern::set_day(int day) {
    if (!is_valid_date(year_, month_, day)) {
        throw std::invalid_argument("Invalid day: " + std::to_string(day));
    }
    day_ = day;
}

int DateIntern::weekday() const {
    // Calculate day of week (0 = Monday, 6 = Sunday)
    std::tm tm = {};
    tm.tm_year = year_ - 1900;
    tm.tm_mon = month_ - 1;
    tm.tm_mday = day_;
    
    std::mktime(&tm);
    return (tm.tm_wday + 6) % 7; // Convert Sunday=0 to Monday=0
}

int DateIntern::isoweekday() const {
    // ISO weekday (1 = Monday, 7 = Sunday)
    return weekday() + 1;
}

std::tuple<int, int, int> DateIntern::isocalendar() const {
    int year = year_;
    int week = 1;
    int weekday = isoweekday();
    
    // Calculate ISO week number
    DateIntern jan1(year, 1, 1);
    int jan1_weekday = jan1.isoweekday();
    
    int days_since_jan1 = ordinal() - jan1.ordinal();
    int week_offset = (jan1_weekday > 4) ? 1 : 0;
    
    week = (days_since_jan1 + jan1_weekday - 1) / 7 + 1 - week_offset;
    
    if (week < 1) {
        // Week belongs to previous year
        year--;
        DateIntern dec31(year, 12, 31);
        auto [prev_year, prev_week, prev_weekday] = dec31.isocalendar();
        week = prev_week;
    } else if (week > 52) {
        // Check if week belongs to next year
        DateIntern dec31(year, 12, 31);
        int dec31_weekday = dec31.isoweekday();
        if (dec31_weekday < 4) {
            year++;
            week = 1;
        }
    }
    
    return std::make_tuple(year, week, weekday);
}

int DateIntern::ordinal() const {
    // Calculate ordinal day number since epoch
    const int epoch_year = 1970;
    const int epoch_ordinal = 719163;
    
    int days = 0;
    
    // Add days for complete years
    for (int y = epoch_year; y < year_; ++y) {
        days += is_leap_year(y) ? 366 : 365;
    }
    
    // Add days for complete months in current year
    for (int m = 1; m < month_; ++m) {
        days += days_in_month(m, year_);
    }
    
    // Add days in current month
    days += day_ - 1;
    
    return epoch_ordinal + days;
}

std::string DateIntern::strftime(const std::string& format) const {
    std::tm tm = {};
    tm.tm_year = year_ - 1900;
    tm.tm_mon = month_ - 1;
    tm.tm_mday = day_;
    
    std::mktime(&tm);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());
    return oss.str();
}

std::string DateIntern::isoformat() const {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << year_ << "-"
        << std::setw(2) << month_ << "-"
        << std::setw(2) << day_;
    return oss.str();
}

std::string DateIntern::to_string() const {
    return isoformat();
}

std::chrono::system_clock::time_point DateIntern::to_time_point() const {
    std::tm tm = {};
    tm.tm_year = year_ - 1900;
    tm.tm_mon = month_ - 1;
    tm.tm_mday = day_;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

DateIntern DateIntern::replace(int year, int month, int day) const {
    int new_year = (year != -1) ? year : year_;
    int new_month = (month != -1) ? month : month_;
    int new_day = (day != -1) ? day : day_;
    
    return DateIntern(new_year, new_month, new_day);
}

DateIntern DateIntern::operator+(int days) const {
    auto tp = to_time_point();
    tp += std::chrono::hours(24 * days);
    return DateIntern(tp);
}

DateIntern DateIntern::operator-(int days) const {
    auto tp = to_time_point();
    tp -= std::chrono::hours(24 * days);
    return DateIntern(tp);
}

int DateIntern::operator-(const DateIntern& other) const {
    return ordinal() - other.ordinal();
}

bool DateIntern::operator==(const DateIntern& other) const {
    return year_ == other.year_ && month_ == other.month_ && day_ == other.day_;
}

bool DateIntern::operator!=(const DateIntern& other) const {
    return !(*this == other);
}

bool DateIntern::operator<(const DateIntern& other) const {
    if (year_ != other.year_) return year_ < other.year_;
    if (month_ != other.month_) return month_ < other.month_;
    return day_ < other.day_;
}

bool DateIntern::operator<=(const DateIntern& other) const {
    return *this < other || *this == other;
}

bool DateIntern::operator>(const DateIntern& other) const {
    return !(*this <= other);
}

bool DateIntern::operator>=(const DateIntern& other) const {
    return !(*this < other);
}

bool DateIntern::is_valid_date(int year, int month, int day) {
    if (month < 1 || month > 12) return false;
    if (day < 1) return false;
    
    return day <= days_in_month(month, year);
}

bool DateIntern::is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DateIntern::days_in_month(int month, int year) {
    int days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    
    return days_per_month[month - 1];
}

void DateIntern::parse_from_string(const std::string& date_str, const std::string& format) {
    std::tm tm = {};
    std::istringstream ss(date_str);
    
    if (format == "%Y-%m-%d") {
        ss >> std::get_time(&tm, "%Y-%m-%d");
    } else if (format == "%d/%m/%Y") {
        ss >> std::get_time(&tm, "%d/%m/%Y");
    } else if (format == "%m/%d/%Y") {
        ss >> std::get_time(&tm, "%m/%d/%Y");
    } else if (format == "%Y%m%d") {
        ss >> std::get_time(&tm, "%Y%m%d");
    } else {
        // Try default format
        ss >> std::get_time(&tm, "%Y-%m-%d");
    }
    
    if (ss.fail()) {
        throw std::invalid_argument("Failed to parse date string: " + date_str);
    }
    
    year_ = tm.tm_year + 1900;
    month_ = tm.tm_mon + 1;
    day_ = tm.tm_mday;
    
    if (!is_valid_date(year_, month_, day_)) {
        throw std::invalid_argument("Invalid date parsed: " + date_str);
    }
}

// TimeIntern implementation
TimeIntern::TimeIntern(int hour, int minute, int second, int microsecond) 
    : hour_(hour), minute_(minute), second_(second), microsecond_(microsecond) {
    if (!is_valid_time(hour, minute, second, microsecond)) {
        throw std::invalid_argument("Invalid time: " + std::to_string(hour) + ":" + 
                                   std::to_string(minute) + ":" + std::to_string(second) + 
                                   "." + std::to_string(microsecond));
    }
}

TimeIntern::TimeIntern(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    
    hour_ = tm.tm_hour;
    minute_ = tm.tm_min;
    second_ = tm.tm_sec;
    
    // Calculate microseconds
    auto duration = tp.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration - seconds);
    microsecond_ = static_cast<int>(microseconds.count());
}

TimeIntern TimeIntern::now() {
    auto now = std::chrono::system_clock::now();
    return TimeIntern(now);
}

int TimeIntern::hour() const {
    return hour_;
}

int TimeIntern::minute() const {
    return minute_;
}

int TimeIntern::second() const {
    return second_;
}

int TimeIntern::microsecond() const {
    return microsecond_;
}

void TimeIntern::set_hour(int hour) {
    if (!is_valid_time(hour, minute_, second_, microsecond_)) {
        throw std::invalid_argument("Invalid hour: " + std::to_string(hour));
    }
    hour_ = hour;
}

void TimeIntern::set_minute(int minute) {
    if (!is_valid_time(hour_, minute, second_, microsecond_)) {
        throw std::invalid_argument("Invalid minute: " + std::to_string(minute));
    }
    minute_ = minute;
}

void TimeIntern::set_second(int second) {
    if (!is_valid_time(hour_, minute_, second, microsecond_)) {
        throw std::invalid_argument("Invalid second: " + std::to_string(second));
    }
    second_ = second;
}

void TimeIntern::set_microsecond(int microsecond) {
    if (!is_valid_time(hour_, minute_, second_, microsecond)) {
        throw std::invalid_argument("Invalid microsecond: " + std::to_string(microsecond));
    }
    microsecond_ = microsecond;
}

std::string TimeIntern::strftime(const std::string& format) const {
    std::tm tm = {};
    tm.tm_hour = hour_;
    tm.tm_min = minute_;
    tm.tm_sec = second_;
    
    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());
    return oss.str();
}

std::string TimeIntern::isoformat() const {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hour_ << ":"
        << std::setw(2) << minute_ << ":"
        << std::setw(2) << second_;
    
    if (microsecond_ > 0) {
        oss << "." << std::setw(6) << microsecond_;
    }
    
    return oss.str();
}

std::string TimeIntern::to_string() const {
    return isoformat();
}

TimeIntern TimeIntern::replace(int hour, int minute, int second, int microsecond) const {
    int new_hour = (hour != -1) ? hour : hour_;
    int new_minute = (minute != -1) ? minute : minute_;
    int new_second = (second != -1) ? second : second_;
    int new_microsecond = (microsecond != -1) ? microsecond : microsecond_;
    
    return TimeIntern(new_hour, new_minute, new_second, new_microsecond);
}

bool TimeIntern::operator==(const TimeIntern& other) const {
    return hour_ == other.hour_ && minute_ == other.minute_ && 
           second_ == other.second_ && microsecond_ == other.microsecond_;
}

bool TimeIntern::operator!=(const TimeIntern& other) const {
    return !(*this == other);
}

bool TimeIntern::operator<(const TimeIntern& other) const {
    if (hour_ != other.hour_) return hour_ < other.hour_;
    if (minute_ != other.minute_) return minute_ < other.minute_;
    if (second_ != other.second_) return second_ < other.second_;
    return microsecond_ < other.microsecond_;
}

bool TimeIntern::operator<=(const TimeIntern& other) const {
    return *this < other || *this == other;
}

bool TimeIntern::operator>(const TimeIntern& other) const {
    return !(*this <= other);
}

bool TimeIntern::operator>=(const TimeIntern& other) const {
    return !(*this < other);
}

bool TimeIntern::is_valid_time(int hour, int minute, int second, int microsecond) {
    return hour >= 0 && hour < 24 &&
           minute >= 0 && minute < 60 &&
           second >= 0 && second < 60 &&
           microsecond >= 0 && microsecond < 1000000;
}

// DateTimeIntern implementation
DateTimeIntern::DateTimeIntern(int year, int month, int day, int hour, int minute, int second, int microsecond)
    : date_(year, month, day), time_(hour, minute, second, microsecond) {}

DateTimeIntern::DateTimeIntern(const DateIntern& date, const TimeIntern& time)
    : date_(date), time_(time) {}

DateTimeIntern::DateTimeIntern(const std::chrono::system_clock::time_point& tp)
    : date_(tp), time_(tp) {}

DateTimeIntern DateTimeIntern::now() {
    auto now = std::chrono::system_clock::now();
    return DateTimeIntern(now);
}

DateTimeIntern DateTimeIntern::combine(const DateIntern& date, const TimeIntern& time) {
    return DateTimeIntern(date, time);
}

const DateIntern& DateTimeIntern::date() const {
    return date_;
}

const TimeIntern& DateTimeIntern::time() const {
    return time_;
}

int DateTimeIntern::year() const {
    return date_.year();
}

int DateTimeIntern::month() const {
    return date_.month();
}

int DateTimeIntern::day() const {
    return date_.day();
}

int DateTimeIntern::hour() const {
    return time_.hour();
}

int DateTimeIntern::minute() const {
    return time_.minute();
}

int DateTimeIntern::second() const {
    return time_.second();
}

int DateTimeIntern::microsecond() const {
    return time_.microsecond();
}

std::string DateTimeIntern::isoformat() const {
    return date_.isoformat() + "T" + time_.isoformat();
}

std::string DateTimeIntern::to_string() const {
    return isoformat();
}

std::chrono::system_clock::time_point DateTimeIntern::to_time_point() const {
    std::tm tm = {};
    tm.tm_year = year() - 1900;
    tm.tm_mon = month() - 1;
    tm.tm_mday = day();
    tm.tm_hour = hour();
    tm.tm_min = minute();
    tm.tm_sec = second();
    
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    tp += std::chrono::microseconds(microsecond());
    
    return tp;
}

DateTimeIntern DateTimeIntern::replace(int year, int month, int day, int hour, int minute, int second, int microsecond) const {
    DateIntern new_date = date_.replace(year, month, day);
    TimeIntern new_time = time_.replace(hour, minute, second, microsecond);
    return DateTimeIntern(new_date, new_time);
}

bool DateTimeIntern::operator==(const DateTimeIntern& other) const {
    return date_ == other.date_ && time_ == other.time_;
}

bool DateTimeIntern::operator!=(const DateTimeIntern& other) const {
    return !(*this == other);
}

bool DateTimeIntern::operator<(const DateTimeIntern& other) const {
    if (date_ != other.date_) return date_ < other.date_;
    return time_ < other.time_;
}

bool DateTimeIntern::operator<=(const DateTimeIntern& other) const {
    return *this < other || *this == other;
}

bool DateTimeIntern::operator>(const DateTimeIntern& other) const {
    return !(*this <= other);
}

bool DateTimeIntern::operator>=(const DateTimeIntern& other) const {
    return !(*this < other);
}

} // namespace utils
} // namespace backtrader