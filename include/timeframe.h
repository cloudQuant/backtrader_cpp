#pragma once

namespace backtrader {

// TimeFrame枚举，定义不同的时间周期
enum class TimeFrame {
    Ticks = 1,
    MicroSeconds = 2,
    Seconds = 3,
    Minutes = 4,
    Days = 5,
    Weeks = 6,
    Months = 7,
    Years = 8,
    NoTimeFrame = 255
};

// TimeFrame相关的辅助函数
inline const char* timeframe_to_string(TimeFrame tf) {
    switch (tf) {
        case TimeFrame::Ticks: return "Ticks";
        case TimeFrame::MicroSeconds: return "MicroSeconds";
        case TimeFrame::Seconds: return "Seconds";
        case TimeFrame::Minutes: return "Minutes";
        case TimeFrame::Days: return "Days";
        case TimeFrame::Weeks: return "Weeks";
        case TimeFrame::Months: return "Months";
        case TimeFrame::Years: return "Years";
        case TimeFrame::NoTimeFrame: return "NoTimeFrame";
        default: return "Unknown";
    }
}

} // namespace backtrader