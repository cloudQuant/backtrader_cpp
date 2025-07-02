#pragma once

#include <string>
#include <sstream>
#include <iomanip>

namespace backtrader {

/**
 * @brief 日期数字转字符串（模拟Python的num2date）
 */
inline std::string num2date(double dt) {
    // 简化实现：假设dt是YYYYMMDD格式的数字
    int date_int = static_cast<int>(dt);
    int year = date_int / 10000;
    int month = (date_int % 10000) / 100;
    int day = date_int % 100;
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << year << "-"
        << std::setw(2) << month << "-" << std::setw(2) << day;
    return oss.str();
}

/**
 * @brief 字符串日期转数字
 */
inline double date2num(const std::string& date_str) {
    // 简化实现：假设date_str是YYYY-MM-DD格式
    if (date_str.length() >= 10) {
        try {
            int year = std::stoi(date_str.substr(0, 4));
            int month = std::stoi(date_str.substr(5, 2));
            int day = std::stoi(date_str.substr(8, 2));
            return year * 10000 + month * 100 + day;
        } catch (...) {
            return 20060102.0;  // 默认日期
        }
    }
    return 20060102.0;  // 默认日期
}

} // namespace backtrader