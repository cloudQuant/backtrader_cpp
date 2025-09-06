#pragma once

#include <chrono>
#include <vector>
#include <string>

namespace backtrader {

// Calendar类，用于处理交易日历
class Calendar {
public:
    Calendar() = default;
    virtual ~Calendar() = default;
    
    // 检查给定日期是否为交易日
    virtual bool is_trading_day(const std::chrono::system_clock::time_point& date) const {
        // 默认实现：周一到周五为交易日
        auto time_t = std::chrono::system_clock::to_time_t(date);
        std::tm* tm = std::localtime(&time_t);
        return tm->tm_wday >= 1 && tm->tm_wday <= 5; // Monday to Friday
    }
    
    // 获取下一个交易日
    virtual std::chrono::system_clock::time_point next_trading_day(
        const std::chrono::system_clock::time_point& date) const {
        auto next_day = date + std::chrono::hours(24);
        while (!is_trading_day(next_day)) {
            next_day += std::chrono::hours(24);
        }
        return next_day;
    }
    
    // 获取前一个交易日
    virtual std::chrono::system_clock::time_point previous_trading_day(
        const std::chrono::system_clock::time_point& date) const {
        auto prev_day = date - std::chrono::hours(24);
        while (!is_trading_day(prev_day)) {
            prev_day -= std::chrono::hours(24);
        }
        return prev_day;
    }
};

// 默认交易日历实例
inline Calendar& get_default_calendar() {
    static Calendar calendar;
    return calendar;
}

} // namespace backtrader