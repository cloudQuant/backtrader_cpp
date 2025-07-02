#pragma once

#include <cstddef>
#include <limits>
#include <cmath>

namespace backtrader {

// 通用类型定义
using size_t = std::size_t;
using index_t = int;

// 常量定义
constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
constexpr double CLOSE_PRICE = 0.0;
constexpr double HIGH_PRICE = 1.0;
constexpr double LOW_PRICE = 2.0;
constexpr double OPEN_PRICE = 3.0;
constexpr double VOLUME = 4.0;

// 便利函数
inline bool isNaN(double value) {
    return std::isnan(value);
}

inline bool isFinite(double value) {
    return std::isfinite(value);
}

inline bool isValid(double value) {
    return std::isfinite(value) && !std::isnan(value);
}

} // namespace backtrader