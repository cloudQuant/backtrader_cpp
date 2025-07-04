#include "../../include/filters/calendardays.h"
#include <chrono>
#include <ctime>

namespace backtrader {
namespace filters {

CalendarDays::CalendarDays(const Params& params) : p(params) {}

void CalendarDays::__call__(std::shared_ptr<DataSeries> data) {
    if (!data || data->lines.empty()) {
        return;
    }
    
    auto& datetime_line = data->lines[0];
    std::vector<size_t> indices_to_remove;
    
    for (size_t i = 0; i < datetime_line.size(); ++i) {
        if (shouldFilterOut(datetime_line[i])) {
            indices_to_remove.push_back(i);
        }
    }
    
    // Remove filtered indices in reverse order to maintain correct indices
    for (auto it = indices_to_remove.rbegin(); it != indices_to_remove.rend(); ++it) {
        removeDataPoint(data, *it);
    }
}

bool CalendarDays::shouldFilterOut(double timestamp) const {
    // Convert timestamp to time structure
    auto time_t_val = static_cast<time_t>(timestamp);
    auto tm = *std::localtime(&time_t_val);
    
    int weekday = tm.tm_wday;  // 0 = Sunday, 1 = Monday, ..., 6 = Saturday
    int month = tm.tm_mon + 1;  // 0-based to 1-based
    int day = tm.tm_mday;
    
    // Check weekends
    if (!p.weekends && (weekday == 0 || weekday == 6)) {
        return true;
    }
    
    // Check specific weekdays
    if (!p.weekdays.empty()) {
        bool found = false;
        for (int allowed_day : p.weekdays) {
            if (weekday == allowed_day) {
                found = true;
                break;
            }
        }
        if (!found) {
            return true;
        }
    }
    
    // Check specific months
    if (!p.months.empty()) {
        bool found = false;
        for (int allowed_month : p.months) {
            if (month == allowed_month) {
                found = true;
                break;
            }
        }
        if (!found) {
            return true;
        }
    }
    
    // Check specific days of month
    if (!p.days.empty()) {
        bool found = false;
        for (int allowed_day : p.days) {
            if (day == allowed_day) {
                found = true;
                break;
            }
        }
        if (!found) {
            return true;
        }
    }
    
    // Check holidays
    if (!p.holidays && isHoliday(tm)) {
        return true;
    }
    
    return false;
}

bool CalendarDays::isHoliday(const std::tm& tm) const {
    int month = tm.tm_mon + 1;
    int day = tm.tm_mday;
    
    // Common US holidays (simplified)
    // New Year's Day
    if (month == 1 && day == 1) {
        return true;
    }
    
    // Independence Day
    if (month == 7 && day == 4) {
        return true;
    }
    
    // Christmas Day
    if (month == 12 && day == 25) {
        return true;
    }
    
    // Memorial Day (last Monday in May)
    if (month == 5 && tm.tm_wday == 1) {
        // Check if it's the last Monday
        int next_monday = day + 7;
        if (next_monday > 31) {
            return true;
        }
    }
    
    // Labor Day (first Monday in September)
    if (month == 9 && tm.tm_wday == 1 && day <= 7) {
        return true;
    }
    
    // Thanksgiving (fourth Thursday in November)
    if (month == 11 && tm.tm_wday == 4) {
        int week_of_month = (day - 1) / 7 + 1;
        if (week_of_month == 4) {
            return true;
        }
    }
    
    return false;
}

void CalendarDays::removeDataPoint(std::shared_ptr<DataSeries> data, size_t index) {
    for (auto& line : data->lines) {
        if (index < line.size()) {
            line.erase(line.begin() + index);
        }
    }
}

} // namespace filters
} // namespace backtrader