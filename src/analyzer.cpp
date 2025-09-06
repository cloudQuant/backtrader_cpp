#include "analyzer.h"
#include "strategy.h"
#include "observer.h"
#include "dataseries.h"
#include "order.h"
#include "trade.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <chrono>
// #include <calendar.h>  // Not available on all systems

namespace backtrader {

Analyzer::Analyzer() {
    create_analysis();
}

void Analyzer::create_analysis() {
    rets.clear();
}

void Analyzer::_start() {
    std::cerr << "Analyzer::_start() - entry, strategy=" << strategy.get() << std::endl;
    _started = true;
    
    // Setup data aliases (matching Python behavior)
    _setup_data_aliases();
    
    // Start this analyzer
    start();
    
    // Start all children
    for (auto& child : _children) {
        child->_start();
    }
    std::cerr << "Analyzer::_start() - completed" << std::endl;
}

void Analyzer::_stop() {
    // Stop this analyzer
    stop();
    
    // Stop all children
    for (auto& child : _children) {
        child->_stop();
    }
    
    _started = false;
}

void Analyzer::_prenext() {
    // Call prenext on all children first
    for (auto& child : _children) {
        child->_prenext();
    }
    
    // Call this analyzer's prenext
    prenext();
}

void Analyzer::_nextstart() {
    // Call nextstart on all children first
    for (auto& child : _children) {
        child->_nextstart();
    }
    
    // Call this analyzer's nextstart
    nextstart();
}

void Analyzer::_next() {
    // Call next on all children first
    for (auto& child : _children) {
        child->_next();
    }
    
    // Call this analyzer's next
    next();
}

// Notification methods
void Analyzer::_notify_cashvalue(double cash, double value) {
    // Notify all children first
    for (auto& child : _children) {
        child->_notify_cashvalue(cash, value);
    }
    
    // Call this analyzer's notification
    notify_cashvalue(cash, value);
}

void Analyzer::_notify_fund(double cash, double value, double fundvalue, double shares) {
    // Notify all children first
    for (auto& child : _children) {
        child->_notify_fund(cash, value, fundvalue, shares);
    }
    
    // Call this analyzer's notification
    notify_fund(cash, value, fundvalue, shares);
}

void Analyzer::_notify_order(std::shared_ptr<Order> order) {
    // Notify all children first
    for (auto& child : _children) {
        child->_notify_order(order);
    }
    
    // Call this analyzer's notification
    notify_order(order);
}

void Analyzer::_notify_trade(std::shared_ptr<Trade> trade) {
    // Notify all children first
    for (auto& child : _children) {
        child->_notify_trade(trade);
    }
    
    // Call this analyzer's notification
    notify_trade(trade);
}

void Analyzer::_register(std::shared_ptr<Analyzer> child) {
    _children.push_back(child);
    child->_parent = shared_from_this();
}

std::vector<std::shared_ptr<Analyzer>> Analyzer::get_children() const {
    return _children;
}

AnalysisResult Analyzer::get_analysis() const {
    return rets;
}

void Analyzer::set_analysis(const AnalysisResult& analysis) {
    rets = analysis;
}

void Analyzer::clear_analysis() {
    rets.clear();
}

void Analyzer::print() const {
    std::cout << "Analysis Results:" << std::endl;
    for (const auto& pair : rets) {
        std::cout << "  " << pair.first << ": " << _analysis_value_to_string(pair.second) << std::endl;
    }
}

void Analyzer::pprint() const {
    // Pretty print with better formatting
    std::cout << "{\n";
    for (const auto& pair : rets) {
        std::cout << "  '" << pair.first << "': " << _analysis_value_to_string(pair.second) << ",\n";
    }
    std::cout << "}" << std::endl;
}

std::string Analyzer::to_string() const {
    std::ostringstream oss;
    oss << "Analyzer[";
    bool first = true;
    for (const auto& pair : rets) {
        if (!first) oss << ", ";
        oss << pair.first << "=" << _analysis_value_to_string(pair.second);
        first = false;
    }
    oss << "]";
    return oss.str();
}

size_t Analyzer::size() const {
    // Return strategy length if available
    if (strategy) {
        // This would need to be implemented in Strategy class
        // For now, return the size of our analysis data
        return rets.size();
    }
    return rets.size();
}

void Analyzer::_setup_data_aliases() {
    if (!datas.empty()) {
        data = datas[0];
        
        // Set up data aliases for first data
        // This would set up data_open, data_high, etc. attributes
        // For C++, we'll skip the complex alias setup for now
        // but the structure is here for future implementation
    }
}

std::string Analyzer::_analysis_value_to_string(const AnalysisValue& value) const {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(4) << v;
            return oss.str();
        } else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<T, std::map<std::string, double>>) {
            std::ostringstream oss;
            oss << "{";
            bool first = true;
            for (const auto& pair : v) {
                if (!first) oss << ", ";
                oss << pair.first << ": " << std::fixed << std::setprecision(4) << pair.second;
                first = false;
            }
            oss << "}";
            return oss.str();
        }
        return "unknown";
    }, value);
}

// TimeFrameAnalyzerBase implementation
TimeFrameAnalyzerBase::TimeFrameAnalyzerBase() 
    : Analyzer(), p() {
    timeframe = p.timeframe;
    compression = p.compression;
}

TimeFrameAnalyzerBase::TimeFrameAnalyzerBase(const Params& params) 
    : Analyzer(), p(params) {
    timeframe = p.timeframe;
    compression = p.compression;
}

void TimeFrameAnalyzerBase::_start() {
    // Override to add specific attributes
    // Set timeframe and compression from data if not specified
    if (timeframe == TimeFrame::NoTimeFrame && !datas.empty()) {
        // This would get timeframe from data
        // For now, default to Days
        timeframe = TimeFrame::Days;
    }
    
    if (compression <= 0 && !datas.empty()) {
        // This would get compression from data
        compression = 1;
    }
    
    // Initialize date comparison values
    auto min_time = std::chrono::system_clock::time_point::min();
    auto [cmp, key] = _get_dt_cmpkey(min_time);
    dtcmp = cmp;
    dtkey = key;
    
    // Call parent implementation
    Analyzer::_start();
}

void TimeFrameAnalyzerBase::_prenext() {
    // Call children first
    for (auto& child : _children) {
        child->_prenext();
    }
    
    // Check if timeframe is over
    if (_dt_over()) {
        on_dt_over();
    }
    
    // Call prenext if enabled
    if (p._doprenext) {
        prenext();
    }
}

void TimeFrameAnalyzerBase::_nextstart() {
    // Call children first
    for (auto& child : _children) {
        child->_nextstart();
    }
    
    // Check if timeframe is over or if prenext is disabled
    if (_dt_over() || !p._doprenext) {
        on_dt_over();
    }
    
    nextstart();
}

void TimeFrameAnalyzerBase::_next() {
    // Call children first
    for (auto& child : _children) {
        child->_next();
    }
    
    // Check if timeframe is over
    bool dt_over = _dt_over();
    // std::cerr << "TimeFrameAnalyzerBase::_next() - _dt_over() returned " << dt_over << std::endl;
    if (dt_over) {
        on_dt_over();
    }
    
    next();
}

bool TimeFrameAnalyzerBase::_dt_over() {
    // If no timeframe, use max values
    if (timeframe == TimeFrame::NoTimeFrame) {
        dtcmp = std::numeric_limits<int64_t>::max();
        dtkey = std::chrono::system_clock::time_point::max();
        return false;
    }
    
    // Get current datetime from strategy
    if (!strategy || strategy->datas.empty()) {
        return false;
    }
    
    // Get datetime from first data feed
    auto data = strategy->datas[0];
    if (!data) {
        return false;
    }
    
    // Cast to DataSeries to access datetime
    auto data_series = std::dynamic_pointer_cast<DataSeries>(data);
    if (!data_series) {
        return false;
    }
    
    // Get current datetime from data
    double dt_double = data_series->datetime(0);
    
    // Debug: print datetime values for first few calls
    // static int debug_calls = 0;
    // if (debug_calls++ < 10) {
    //     std::cerr << "TimeFrameAnalyzerBase::_dt_over() - dt_double=" << dt_double << std::endl;
    // }
    
    auto time_t = static_cast<std::time_t>(dt_double);
    auto dt = std::chrono::system_clock::from_time_t(time_t);
    
    auto [cmp, key] = _get_dt_cmpkey(dt);
    
    // Debug: print comparison values
    // if (debug_calls <= 10) {
    //     std::cerr << "  Current dtcmp=" << dtcmp << ", new cmp=" << cmp << std::endl;
    // }
    
    // Check if we've moved to a new timeframe
    if (dtcmp == 0 || cmp > dtcmp) {
        dtkey1 = dtkey;
        dtkey = key;
        dtcmp1 = dtcmp;
        dtcmp = cmp;
        
        // if (debug_calls <= 10) {
        //     std::cerr << "  TimeFrame boundary crossed! Returning true" << std::endl;
        // }
        
        return true;
    }
    
    return false;
}

std::pair<int64_t, std::chrono::system_clock::time_point> 
TimeFrameAnalyzerBase::_get_dt_cmpkey(std::chrono::system_clock::time_point dt) {
    if (timeframe == TimeFrame::NoTimeFrame) {
        return {0, std::chrono::system_clock::time_point{}};
    }
    
    auto tm = to_tm(dt);
    int64_t dtcmp = 0;
    std::chrono::system_clock::time_point dtkey;
    
    switch (timeframe) {
        case TimeFrame::Years: {
            dtcmp = tm.tm_year + 1900;
            tm.tm_mon = 11;  // December
            tm.tm_mday = 31;
            dtkey = from_tm(tm);
            break;
        }
        case TimeFrame::Months: {
            dtcmp = (tm.tm_year + 1900) * 100 + (tm.tm_mon + 1);
            // Get last day of month
            tm.tm_mday = 31;  // Simplified, should use actual last day
            dtkey = from_tm(tm);
            break;
        }
        case TimeFrame::Weeks: {
            // ISO week calculation (simplified)
            int week = tm.tm_yday / 7;
            dtcmp = (tm.tm_year + 1900) * 1000 + week;
            // Get Sunday of the week
            int days_to_sunday = 7 - tm.tm_wday;
            auto sunday = dt + std::chrono::hours(24 * days_to_sunday);
            dtkey = sunday;
            break;
        }
        case TimeFrame::Days: {
            dtcmp = (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
            tm.tm_hour = 0;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            dtkey = from_tm(tm);
            break;
        }
        default: {
            return _get_subday_cmpkey(dt);
        }
    }
    
    return {dtcmp, dtkey};
}

std::pair<int64_t, std::chrono::system_clock::time_point> 
TimeFrameAnalyzerBase::_get_subday_cmpkey(std::chrono::system_clock::time_point dt) {
    auto tm = to_tm(dt);
    
    // Calculate intraday position
    int64_t point = tm.tm_hour * 60 + tm.tm_min;
    
    if (timeframe < TimeFrame::Minutes) {
        point = point * 60 + tm.tm_sec;
    }
    
    // Apply compression
    point = point / compression;
    
    // Move to next boundary
    point += 1;
    
    // Restore point
    point *= compression;
    
    // Calculate new time components
    int ph = 0, pm = 0, ps = 0;
    int extradays = 0;
    
    if (timeframe == TimeFrame::Minutes) {
        ph = point / 60;
        pm = point % 60;
        ps = 0;
    } else if (timeframe == TimeFrame::Seconds) {
        ph = point / 3600;
        pm = (point % 3600) / 60;
        ps = point % 60;
    }
    
    if (ph > 23) {
        extradays = ph / 24;
        ph %= 24;
    }
    
    // Adjust time
    auto result_dt = dt;
    if (extradays > 0) {
        result_dt += std::chrono::hours(24 * extradays);
    }
    
    auto result_tm = to_tm(result_dt);
    result_tm.tm_hour = ph;
    result_tm.tm_min = pm;
    result_tm.tm_sec = ps;
    
    auto dtkey = from_tm(result_tm);
    
    // Adjust backwards by one unit
    if (timeframe == TimeFrame::Minutes) {
        dtkey -= std::chrono::minutes(1);
    } else if (timeframe == TimeFrame::Seconds) {
        dtkey -= std::chrono::seconds(1);
    }
    
    auto dtcmp = std::chrono::duration_cast<std::chrono::seconds>(dtkey.time_since_epoch()).count();
    
    return {dtcmp, dtkey};
}

std::tm TimeFrameAnalyzerBase::to_tm(std::chrono::system_clock::time_point tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    return *std::localtime(&time_t);
}

std::chrono::system_clock::time_point TimeFrameAnalyzerBase::from_tm(const std::tm& tm) {
    auto time_t = std::mktime(const_cast<std::tm*>(&tm));
    return std::chrono::system_clock::from_time_t(time_t);
}

} // namespace backtrader