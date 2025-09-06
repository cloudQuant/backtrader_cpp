#pragma once

#include "metabase.h"
#include <chrono>
#include <vector>
#include <tuple>
#include <memory>

namespace backtrader {

// Weekday constants
enum class Weekday : int {
    MONDAY = 0,
    TUESDAY = 1,
    WEDNESDAY = 2,
    THURSDAY = 3,
    FRIDAY = 4,
    SATURDAY = 5,
    SUNDAY = 6
};

// ISO weekday constants (Monday = 1, Sunday = 7)
enum class ISOWeekday : int {
    ISONODAY = 0,
    ISOMONDAY = 1,
    ISOTUESDAY = 2,
    ISOWEDNESDAY = 3,
    ISOTHURSDAY = 4,
    ISOFRIDAY = 5,
    ISOSATURDAY = 6,
    ISOSUNDAY = 7
};

// Weekend days
extern const std::vector<Weekday> WEEKEND;
extern const std::vector<ISOWeekday> ISOWEEKEND;

// One day duration
extern const std::chrono::hours ONEDAY;

// Maximum time in a day
extern const std::chrono::system_clock::time_point TIME_MAX;

// ISO calendar structure (year, week, day)
struct ISOCalendar {
    int year;
    int week;
    int day;
    
    ISOCalendar(int y = 0, int w = 0, int d = 0) : year(y), week(w), day(d) {}
};

// Trading schedule structure
struct TradingSchedule {
    std::chrono::system_clock::time_point open_time;
    std::chrono::system_clock::time_point close_time;
    
    TradingSchedule() = default;
    TradingSchedule(const std::chrono::system_clock::time_point& open, 
                   const std::chrono::system_clock::time_point& close)
        : open_time(open), close_time(close) {}
};

// Base trading calendar class
class TradingCalendarBase {
public:
    virtual ~TradingCalendarBase() = default;
    
    // Pure virtual methods
    virtual std::pair<std::chrono::system_clock::time_point, ISOCalendar> 
        nextday_info(const std::chrono::system_clock::time_point& day) = 0;
    
    virtual TradingSchedule 
        schedule(const std::chrono::system_clock::time_point& day) = 0;
    
    // Implemented methods
    std::chrono::system_clock::time_point 
        nextday(const std::chrono::system_clock::time_point& day);
    
    int nextday_week(const std::chrono::system_clock::time_point& day);
    
    bool last_weekday(const std::chrono::system_clock::time_point& day);
    
    bool last_monthday(const std::chrono::system_clock::time_point& day);
    
    bool last_yearday(const std::chrono::system_clock::time_point& day);
    
protected:
    ISOCalendar get_iso_calendar(const std::chrono::system_clock::time_point& day) const;
    Weekday get_weekday(const std::chrono::system_clock::time_point& day) const;
    bool is_weekend(const std::chrono::system_clock::time_point& day) const;
};

// Standard trading calendar implementation
class TradingCalendar : public TradingCalendarBase {
public:
    struct Params {
        std::chrono::system_clock::time_point open_time;
        std::chrono::system_clock::time_point close_time;
        std::vector<Weekday> trading_days = {
            Weekday::MONDAY, Weekday::TUESDAY, Weekday::WEDNESDAY, 
            Weekday::THURSDAY, Weekday::FRIDAY
        };
        std::vector<std::chrono::system_clock::time_point> holidays;
    } params;
    
    TradingCalendar();
    virtual ~TradingCalendar() = default;
    
    std::pair<std::chrono::system_clock::time_point, ISOCalendar> 
        nextday_info(const std::chrono::system_clock::time_point& day) override;
    
    TradingSchedule 
        schedule(const std::chrono::system_clock::time_point& day) override;
    
    // Additional methods
    void add_holiday(const std::chrono::system_clock::time_point& holiday);
    void remove_holiday(const std::chrono::system_clock::time_point& holiday);
    bool is_holiday(const std::chrono::system_clock::time_point& day) const;
    bool is_trading_day(const std::chrono::system_clock::time_point& day) const;
    
    void set_trading_hours(const std::chrono::system_clock::time_point& open,
                          const std::chrono::system_clock::time_point& close);
    
private:
    std::chrono::system_clock::time_point 
        find_next_trading_day(const std::chrono::system_clock::time_point& day) const;
};

// Pandas market calendar integration (placeholder)
class PandasMarketCalendar : public TradingCalendarBase {
public:
    struct Params {
        std::string calendar_name = "NYSE";
    } params;
    
    PandasMarketCalendar(const std::string& name = "NYSE");
    virtual ~PandasMarketCalendar() = default;
    
    std::pair<std::chrono::system_clock::time_point, ISOCalendar> 
        nextday_info(const std::chrono::system_clock::time_point& day) override;
    
    TradingSchedule 
        schedule(const std::chrono::system_clock::time_point& day) override;
    
private:
    std::string calendar_name_;
    // Placeholder for pandas integration
    // In a real implementation, this would interface with pandas_market_calendars
};

// Utility functions
ISOCalendar to_iso_calendar(const std::chrono::system_clock::time_point& tp);
Weekday to_weekday(const std::chrono::system_clock::time_point& tp);
std::chrono::system_clock::time_point from_iso_calendar(const ISOCalendar& cal);

// Date arithmetic
std::chrono::system_clock::time_point add_days(const std::chrono::system_clock::time_point& tp, int days);
std::chrono::system_clock::time_point add_weeks(const std::chrono::system_clock::time_point& tp, int weeks);
std::chrono::system_clock::time_point add_months(const std::chrono::system_clock::time_point& tp, int months);
std::chrono::system_clock::time_point add_years(const std::chrono::system_clock::time_point& tp, int years);

// Factory functions
std::shared_ptr<TradingCalendar> create_standard_calendar();
std::shared_ptr<PandasMarketCalendar> create_pandas_calendar(const std::string& name = "NYSE");

} // namespace backtrader