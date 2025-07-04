#include "timer.h"
#include "strategy.h"
#include <sstream>
#include <algorithm>

namespace backtrader {

Timer::Timer() {
    next_trigger = std::chrono::system_clock::now();
}

bool Timer::check(std::chrono::system_clock::time_point current_time) {
    if (!active || !started_) {
        return false;
    }
    
    switch (params.timetype) {
        case TimerType::SESSION_TIME:
            return check_session_time(current_time);
        case TimerType::SESSION_START:
            return check_session_start(current_time);
        case TimerType::SESSION_END:
            return check_session_end(current_time);
        default:
            return false;
    }
}

void Timer::trigger() {
    if (params.callback) {
        execute_callback();
    }
    
    // Calculate next trigger time if repeating
    if (params.repeat.count() > 0) {
        next_trigger = calculate_next_trigger(std::chrono::system_clock::now());
    } else {
        active = false; // One-time timer
    }
}

void Timer::start() {
    started_ = true;
    active = true;
    next_trigger = calculate_next_trigger(std::chrono::system_clock::now());
}

void Timer::stop() {
    started_ = false;
    active = false;
}

bool Timer::is_active() const {
    return active && started_;
}

std::chrono::system_clock::time_point Timer::calculate_next_trigger(
    std::chrono::system_clock::time_point current_time) {
    
    auto next_time = params.when;
    
    // Apply offset
    next_time += params.offset;
    
    // If repeating, add repeat interval until we're in the future
    if (params.repeat.count() > 0) {
        while (next_time <= current_time) {
            next_time += params.repeat;
        }
    }
    
    // Adjust for weekdays if specified
    if (!params.weekdays.empty()) {
        next_time = adjust_for_weekday(next_time);
    }
    
    // Adjust for month days if specified
    if (params.monthdays >= 0) {
        next_time = adjust_for_monthday(next_time);
    }
    
    return next_time;
}

void Timer::execute_callback() {
    if (params.callback) {
        try {
            params.callback();
        } catch (const std::exception& e) {
            // Log error or handle callback failure
            // For now, just continue execution
        }
    }
}

std::string Timer::to_string() const {
    std::ostringstream oss;
    oss << "Timer[" << params.tid << "] ";
    oss << "Type: ";
    switch (params.timetype) {
        case TimerType::SESSION_TIME:
            oss << "SESSION_TIME";
            break;
        case TimerType::SESSION_START:
            oss << "SESSION_START";
            break;
        case TimerType::SESSION_END:
            oss << "SESSION_END";
            break;
    }
    oss << " Active: " << (active ? "Yes" : "No");
    return oss.str();
}

bool Timer::check_session_time(std::chrono::system_clock::time_point current_time) {
    return current_time >= next_trigger;
}

bool Timer::check_session_start(std::chrono::system_clock::time_point current_time) {
    // Check if we're at the start of a session
    // This would need to be implemented based on market session logic
    return current_time >= next_trigger;
}

bool Timer::check_session_end(std::chrono::system_clock::time_point current_time) {
    // Check if we're at the end of a session
    // This would need to be implemented based on market session logic
    return current_time >= next_trigger;
}

bool Timer::is_valid_weekday(std::chrono::system_clock::time_point time_point) {
    if (params.weekdays.empty()) {
        return true;
    }
    
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    auto tm = *std::localtime(&time_t);
    int weekday = tm.tm_wday; // 0 = Sunday, 1 = Monday, etc.
    
    return std::find(params.weekdays.begin(), params.weekdays.end(), weekday) 
           != params.weekdays.end();
}

bool Timer::is_valid_monthday(std::chrono::system_clock::time_point time_point) {
    if (params.monthdays < 0) {
        return true;
    }
    
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    auto tm = *std::localtime(&time_t);
    
    return tm.tm_mday == params.monthdays;
}

std::chrono::system_clock::time_point Timer::adjust_for_weekday(
    std::chrono::system_clock::time_point time_point) {
    
    if (is_valid_weekday(time_point)) {
        return time_point;
    }
    
    // Find next valid weekday
    auto adjusted = time_point;
    for (int i = 0; i < 7; ++i) { // Max 7 days to find a valid weekday
        adjusted += std::chrono::hours(24);
        if (is_valid_weekday(adjusted)) {
            return adjusted;
        }
    }
    
    return time_point; // Fallback
}

std::chrono::system_clock::time_point Timer::adjust_for_monthday(
    std::chrono::system_clock::time_point time_point) {
    
    if (is_valid_monthday(time_point)) {
        return time_point;
    }
    
    // This would need more complex logic for month day adjustments
    return time_point;
}

// TimerManager implementation
void TimerManager::add_timer(std::shared_ptr<Timer> timer) {
    timers_.push_back(timer);
}

void TimerManager::remove_timer(std::shared_ptr<Timer> timer) {
    timers_.erase(
        std::remove(timers_.begin(), timers_.end(), timer),
        timers_.end()
    );
}

void TimerManager::clear_timers() {
    timers_.clear();
}

void TimerManager::check_timers(std::chrono::system_clock::time_point current_time) {
    for (auto& timer : timers_) {
        if (timer->check(current_time)) {
            timer->trigger();
        }
    }
}

std::vector<std::shared_ptr<Timer>> TimerManager::get_active_timers() const {
    std::vector<std::shared_ptr<Timer>> active_timers;
    for (const auto& timer : timers_) {
        if (timer->is_active()) {
            active_timers.push_back(timer);
        }
    }
    return active_timers;
}

size_t TimerManager::timer_count() const {
    return timers_.size();
}

// Factory functions
std::shared_ptr<Timer> create_timer(
    std::chrono::system_clock::time_point when,
    std::function<void()> callback,
    TimerType timetype) {
    
    auto timer = std::make_shared<Timer>();
    timer->params.when = when;
    timer->params.callback = callback;
    timer->params.timetype = timetype;
    return timer;
}

std::shared_ptr<Timer> create_session_timer(
    std::chrono::system_clock::time_point when,
    std::function<void()> callback) {
    
    return create_timer(when, callback, TimerType::SESSION_TIME);
}

std::shared_ptr<Timer> create_session_start_timer(
    std::function<void()> callback) {
    
    return create_timer(std::chrono::system_clock::now(), callback, TimerType::SESSION_START);
}

std::shared_ptr<Timer> create_session_end_timer(
    std::function<void()> callback) {
    
    return create_timer(std::chrono::system_clock::now(), callback, TimerType::SESSION_END);
}

} // namespace backtrader