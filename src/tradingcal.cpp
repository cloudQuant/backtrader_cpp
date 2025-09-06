#include "tradingcal.h"
#include <algorithm>
#include <ctime>
#include <iomanip>

namespace backtrader {

// Constants
const std::vector<Weekday> WEEKEND = {Weekday::SATURDAY, Weekday::SUNDAY};
const std::vector<ISOWeekday> ISOWEEKEND = {ISOWeekday::ISOSATURDAY, ISOWeekday::ISOSUNDAY};
const std::chrono::hours ONEDAY = std::chrono::hours(24);

// TradingCalendarBase implementation
std::chrono::system_clock::time_point 
TradingCalendarBase::nextday(const std::chrono::system_clock::time_point& day) {
    return nextday_info(day).first;
}

int TradingCalendarBase::nextday_week(const std::chrono::system_clock::time_point& day) {
    return nextday_info(day).second.week;
}

bool TradingCalendarBase::last_weekday(const std::chrono::system_clock::time_point& day) {
    auto current_week = get_iso_calendar(day).week;
    auto next_week = nextday_info(day).second.week;
    return current_week != next_week;
}

bool TradingCalendarBase::last_monthday(const std::chrono::system_clock::time_point& day) {
    auto time_t = std::chrono::system_clock::to_time_t(day);
    auto tm = *std::gmtime(&time_t);
    int current_month = tm.tm_mon;
    
    auto next_day = nextday(day);
    auto next_time_t = std::chrono::system_clock::to_time_t(next_day);
    auto next_tm = *std::gmtime(&next_time_t);
    int next_month = next_tm.tm_mon;
    
    return current_month != next_month;
}

bool TradingCalendarBase::last_yearday(const std::chrono::system_clock::time_point& day) {
    auto time_t = std::chrono::system_clock::to_time_t(day);
    auto tm = *std::gmtime(&time_t);
    int current_year = tm.tm_year;
    
    auto next_day = nextday(day);
    auto next_time_t = std::chrono::system_clock::to_time_t(next_day);
    auto next_tm = *std::gmtime(&next_time_t);
    int next_year = next_tm.tm_year;
    
    return current_year != next_year;
}

ISOCalendar TradingCalendarBase::get_iso_calendar(const std::chrono::system_clock::time_point& day) const {
    return to_iso_calendar(day);
}

Weekday TradingCalendarBase::get_weekday(const std::chrono::system_clock::time_point& day) const {
    return to_weekday(day);
}

bool TradingCalendarBase::is_weekend(const std::chrono::system_clock::time_point& day) const {
    auto wd = get_weekday(day);
    return std::find(WEEKEND.begin(), WEEKEND.end(), wd) != WEEKEND.end();
}

// TradingCalendar implementation
TradingCalendar::TradingCalendar() {
    // Set default trading hours (9:30 AM - 4:00 PM EST)
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::gmtime(&time_t);
    
    // 9:30 AM
    tm.tm_hour = 9;
    tm.tm_min = 30;
    tm.tm_sec = 0;
    params.open_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    
    // 4:00 PM
    tm.tm_hour = 16;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    params.close_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::pair<std::chrono::system_clock::time_point, ISOCalendar> 
TradingCalendar::nextday_info(const std::chrono::system_clock::time_point& day) {
    auto next_day = find_next_trading_day(day);
    auto iso_cal = to_iso_calendar(next_day);
    return std::make_pair(next_day, iso_cal);
}

TradingSchedule 
TradingCalendar::schedule(const std::chrono::system_clock::time_point& day) {
    if (!is_trading_day(day)) {
        // Return empty schedule for non-trading days
        return TradingSchedule();
    }
    
    // Adjust open/close times to the specific day
    auto time_t = std::chrono::system_clock::to_time_t(day);
    auto tm = *std::gmtime(&time_t);
    
    auto open_time_t = std::chrono::system_clock::to_time_t(params.open_time);
    auto open_tm = *std::gmtime(&open_time_t);
    
    auto close_time_t = std::chrono::system_clock::to_time_t(params.close_time);
    auto close_tm = *std::gmtime(&close_time_t);
    
    // Set the date to the specific day but keep the time
    open_tm.tm_year = tm.tm_year;
    open_tm.tm_mon = tm.tm_mon;
    open_tm.tm_mday = tm.tm_mday;
    
    close_tm.tm_year = tm.tm_year;
    close_tm.tm_mon = tm.tm_mon;
    close_tm.tm_mday = tm.tm_mday;
    
    auto day_open = std::chrono::system_clock::from_time_t(std::mktime(&open_tm));
    auto day_close = std::chrono::system_clock::from_time_t(std::mktime(&close_tm));
    
    return TradingSchedule(day_open, day_close);
}

void TradingCalendar::add_holiday(const std::chrono::system_clock::time_point& holiday) {
    params.holidays.push_back(holiday);
    std::sort(params.holidays.begin(), params.holidays.end());
}

void TradingCalendar::remove_holiday(const std::chrono::system_clock::time_point& holiday) {
    auto it = std::find(params.holidays.begin(), params.holidays.end(), holiday);
    if (it != params.holidays.end()) {
        params.holidays.erase(it);
    }
}

bool TradingCalendar::is_holiday(const std::chrono::system_clock::time_point& day) const {
    // Compare only dates, not times
    auto day_date = std::chrono::floor<std::chrono::days>(day);
    
    for (const auto& holiday : params.holidays) {
        auto holiday_date = std::chrono::floor<std::chrono::days>(holiday);
        if (day_date == holiday_date) {
            return true;
        }
    }
    return false;
}

bool TradingCalendar::is_trading_day(const std::chrono::system_clock::time_point& day) const {
    // Check if it's a weekend
    if (is_weekend(day)) {
        return false;
    }
    
    // Check if it's a holiday
    if (is_holiday(day)) {
        return false;
    }
    
    // Check if it's in the trading days list
    auto wd = get_weekday(day);
    return std::find(params.trading_days.begin(), params.trading_days.end(), wd) 
           != params.trading_days.end();
}

void TradingCalendar::set_trading_hours(const std::chrono::system_clock::time_point& open,
                                       const std::chrono::system_clock::time_point& close) {
    params.open_time = open;
    params.close_time = close;
}

std::chrono::system_clock::time_point 
TradingCalendar::find_next_trading_day(const std::chrono::system_clock::time_point& day) const {
    auto next_day = add_days(day, 1);
    
    // Keep looking until we find a trading day
    while (!is_trading_day(next_day)) {
        next_day = add_days(next_day, 1);
    }
    
    return next_day;
}

// PandasMarketCalendar implementation (placeholder)
PandasMarketCalendar::PandasMarketCalendar(const std::string& name) 
    : calendar_name_(name) {
    params.calendar_name = name;
}

std::pair<std::chrono::system_clock::time_point, ISOCalendar> 
PandasMarketCalendar::nextday_info(const std::chrono::system_clock::time_point& day) {
    // Placeholder implementation - would need actual pandas integration
    // For now, just use standard weekday logic
    auto next_day = day;
    do {
        next_day = add_days(next_day, 1);
    } while (to_weekday(next_day) == Weekday::SATURDAY || 
             to_weekday(next_day) == Weekday::SUNDAY);
    
    auto iso_cal = to_iso_calendar(next_day);
    return std::make_pair(next_day, iso_cal);
}

TradingSchedule 
PandasMarketCalendar::schedule(const std::chrono::system_clock::time_point& day) {
    // Placeholder implementation
    // Default NYSE hours: 9:30 AM - 4:00 PM EST
    auto time_t = std::chrono::system_clock::to_time_t(day);
    auto tm = *std::gmtime(&time_t);
    
    tm.tm_hour = 9;
    tm.tm_min = 30;
    tm.tm_sec = 0;
    auto open = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    
    tm.tm_hour = 16;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    auto close = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    
    return TradingSchedule(open, close);
}

// Utility functions
ISOCalendar to_iso_calendar(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::gmtime(&time_t);
    
    // Calculate ISO week
    // This is a simplified implementation
    int year = tm.tm_year + 1900;
    int yday = tm.tm_yday + 1;
    int wday = tm.tm_wday == 0 ? 7 : tm.tm_wday; // Convert Sunday=0 to Sunday=7
    
    // ISO week calculation (simplified)
    int week = (yday - wday + 10) / 7;
    
    return ISOCalendar(year, week, wday);
}

Weekday to_weekday(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::gmtime(&time_t);
    
    // tm_wday: Sunday=0, Monday=1, ..., Saturday=6
    // Convert to our enum: Monday=0, Tuesday=1, ..., Sunday=6
    int wday = (tm.tm_wday + 6) % 7;
    return static_cast<Weekday>(wday);
}

std::chrono::system_clock::time_point from_iso_calendar(const ISOCalendar& cal) {
    // Simplified implementation
    std::tm tm = {};
    tm.tm_year = cal.year - 1900;
    tm.tm_mon = 0; // January
    tm.tm_mday = 1; // 1st day
    
    // Add weeks and days (simplified)
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    tp += std::chrono::hours(24 * 7 * (cal.week - 1)); // Add weeks
    tp += std::chrono::hours(24 * (cal.day - 1));      // Add days
    
    return tp;
}

std::chrono::system_clock::time_point add_days(const std::chrono::system_clock::time_point& tp, int days) {
    return tp + std::chrono::hours(24 * days);
}

std::chrono::system_clock::time_point add_weeks(const std::chrono::system_clock::time_point& tp, int weeks) {
    return tp + std::chrono::hours(24 * 7 * weeks);
}

std::chrono::system_clock::time_point add_months(const std::chrono::system_clock::time_point& tp, int months) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::gmtime(&time_t);
    
    tm.tm_mon += months;
    
    // Handle year overflow
    while (tm.tm_mon >= 12) {
        tm.tm_mon -= 12;
        tm.tm_year++;
    }
    while (tm.tm_mon < 0) {
        tm.tm_mon += 12;
        tm.tm_year--;
    }
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::chrono::system_clock::time_point add_years(const std::chrono::system_clock::time_point& tp, int years) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::gmtime(&time_t);
    
    tm.tm_year += years;
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// Factory functions
std::shared_ptr<TradingCalendar> create_standard_calendar() {
    return std::make_shared<TradingCalendar>();
}

std::shared_ptr<PandasMarketCalendar> create_pandas_calendar(const std::string& name) {
    return std::make_shared<PandasMarketCalendar>(name);
}

} // namespace backtrader