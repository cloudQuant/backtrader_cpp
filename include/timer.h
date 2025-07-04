#pragma once

#include "metabase.h"
#include <memory>
#include <chrono>
#include <functional>
#include <vector>

namespace backtrader {

// Timer types
enum class TimerType {
    SESSION_TIME = 0,
    SESSION_START = 1,
    SESSION_END = 2
};

// Forward declaration
class Strategy;

// Timer class
class Timer {
public:
    // Parameters
    struct Params {
        int tid = -1;
        std::shared_ptr<Strategy> owner = nullptr;
        std::chrono::system_clock::time_point when;
        TimerType timetype = TimerType::SESSION_TIME;
        std::chrono::system_clock::duration offset;
        std::chrono::system_clock::duration repeat;
        std::vector<int> weekdays;
        int weekcarry = false;
        int monthdays = -1;
        int monthcarry = true;
        bool allow_creation = true;
        bool cheat = false;
        std::string tz = "";
        std::function<void()> callback = nullptr;
        void* args = nullptr;
        void* kwargs = nullptr;
    } params;
    
    Timer();
    virtual ~Timer() = default;
    
    // Timer state
    bool active = true;
    std::chrono::system_clock::time_point next_trigger;
    
    // Timer management
    bool check(std::chrono::system_clock::time_point current_time);
    void trigger();
    void start();
    void stop();
    bool is_active() const;
    
    // Time calculations
    std::chrono::system_clock::time_point calculate_next_trigger(
        std::chrono::system_clock::time_point current_time);
    
    // Callback execution
    void execute_callback();
    
    // String representation
    std::string to_string() const;
    
private:
    bool started_ = false;
    
    // Helper methods for different timer types
    bool check_session_time(std::chrono::system_clock::time_point current_time);
    bool check_session_start(std::chrono::system_clock::time_point current_time);
    bool check_session_end(std::chrono::system_clock::time_point current_time);
    
    // Date/time helpers
    bool is_valid_weekday(std::chrono::system_clock::time_point time_point);
    bool is_valid_monthday(std::chrono::system_clock::time_point time_point);
    std::chrono::system_clock::time_point adjust_for_weekday(
        std::chrono::system_clock::time_point time_point);
    std::chrono::system_clock::time_point adjust_for_monthday(
        std::chrono::system_clock::time_point time_point);
};

// Timer manager for handling multiple timers
class TimerManager {
public:
    TimerManager() = default;
    virtual ~TimerManager() = default;
    
    // Timer management
    void add_timer(std::shared_ptr<Timer> timer);
    void remove_timer(std::shared_ptr<Timer> timer);
    void clear_timers();
    
    // Check all timers
    void check_timers(std::chrono::system_clock::time_point current_time);
    
    // Get active timers
    std::vector<std::shared_ptr<Timer>> get_active_timers() const;
    size_t timer_count() const;
    
private:
    std::vector<std::shared_ptr<Timer>> timers_;
};

// Timer factory functions
std::shared_ptr<Timer> create_timer(
    std::chrono::system_clock::time_point when,
    std::function<void()> callback = nullptr,
    TimerType timetype = TimerType::SESSION_TIME);

std::shared_ptr<Timer> create_session_timer(
    std::chrono::system_clock::time_point when,
    std::function<void()> callback = nullptr);

std::shared_ptr<Timer> create_session_start_timer(
    std::function<void()> callback = nullptr);

std::shared_ptr<Timer> create_session_end_timer(
    std::function<void()> callback = nullptr);

} // namespace backtrader