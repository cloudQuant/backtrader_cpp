#include "../../include/filters/session.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace filters {

// Initialize MAXDATE to a very large date
const std::chrono::system_clock::time_point SessionFiller::MAXDATE = 
    std::chrono::system_clock::time_point::max();

SessionFiller::SessionFiller(std::shared_ptr<AbstractDataBase> data, const Params& params) 
    : p(params), seenbar_(false), sessend_(MAXDATE) {
    
    // Calculate timeframe delta based on data's timeframe
    if (data) {
        tdframe_ = calculate_timeframe_delta(data->get_timeframe());
        
        // Calculate time unit based on compression
        auto compression = data->get_compression();
        tdunit_ = tdframe_ * compression;
        
        std::cout << "SessionFiller initialized with timeframe delta: " 
                  << tdframe_.count() << "ms, unit: " << tdunit_.count() << "ms" << std::endl;
    }
}

bool SessionFiller::operator()(std::shared_ptr<AbstractDataBase> data) {
    if (!data) {
        return false;
    }
    
    // Get current bar datetime
    auto current_time = data->get_datetime();
    
    // Calculate session limits for this bar
    calculate_session_limits(data);
    
    // Check if this is the first bar we've seen
    if (!seenbar_) {
        seenbar_ = true;
        
        // If skip_first_fill is true, don't fill from session start to first bar
        if (p.skip_first_fill) {
            std::cout << "First bar seen, skipping fill from session start" << std::endl;
            return true;
        }
    }
    
    // Check if current bar is within session
    if (!is_within_session(current_time)) {
        std::cout << "Bar is outside session time, skipping" << std::endl;
        return true;
    }
    
    // Calculate if we need to fill bars between previous bar and current bar
    auto previous_time = data->get_previous_datetime();
    
    if (previous_time != std::chrono::system_clock::time_point{}) {
        // Calculate expected next bar time after previous bar
        auto expected_next_time = previous_time + tdunit_;
        
        // Fill bars if there's a gap
        if (current_time > expected_next_time) {
            fill_bars_up_to(current_time, data);
        }
    }
    
    return true;
}

std::chrono::milliseconds SessionFiller::calculate_timeframe_delta(TimeFrame::Value timeframe) const {
    switch (timeframe) {
        case TimeFrame::Ticks:
            return std::chrono::milliseconds(1);
            
        case TimeFrame::MicroSeconds:
            return std::chrono::microseconds(1);
            
        case TimeFrame::Seconds:
            return std::chrono::seconds(1);
            
        case TimeFrame::Minutes:
            return std::chrono::minutes(1);
            
        case TimeFrame::Days:
            return std::chrono::hours(24);
            
        case TimeFrame::Weeks:
            return std::chrono::hours(24 * 7);
            
        case TimeFrame::Months:
            return std::chrono::hours(24 * 30); // Approximation
            
        case TimeFrame::Years:
            return std::chrono::hours(24 * 365); // Approximation
            
        default:
            return std::chrono::minutes(1); // Default to 1 minute
    }
}

void SessionFiller::calculate_session_limits(std::shared_ptr<AbstractDataBase> data) {
    // Get session information from data source
    auto session_start = data->get_session_start();
    auto session_end = data->get_session_end();
    
    if (session_end != std::chrono::system_clock::time_point{}) {
        sessend_ = session_end;
    } else {
        // If no session end defined, use maximum date
        sessend_ = MAXDATE;
    }
    
    // Log session information
    auto session_start_time_t = std::chrono::system_clock::to_time_t(session_start);
    auto session_end_time_t = std::chrono::system_clock::to_time_t(sessend_);
    
    std::cout << "Session limits - Start: " << std::ctime(&session_start_time_t)
              << "End: " << (sessend_ == MAXDATE ? "No limit" : std::ctime(&session_end_time_t)) << std::endl;
}

bool SessionFiller::is_within_session(std::chrono::system_clock::time_point bar_time) const {
    // Simple check - in practice this would be more complex with session start/end times
    return bar_time <= sessend_;
}

void SessionFiller::fill_bars_up_to(std::chrono::system_clock::time_point target_time, 
                                   std::shared_ptr<AbstractDataBase> data) {
    
    auto previous_time = data->get_previous_datetime();
    auto current_bars_to_fill = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(target_time - previous_time).count() / 
        tdunit_.count()) - 1;
    
    if (current_bars_to_fill <= 0) {
        return; // No bars to fill
    }
    
    std::cout << "Filling " << current_bars_to_fill << " bars between " 
              << std::chrono::duration_cast<std::chrono::seconds>(previous_time.time_since_epoch()).count()
              << " and " << std::chrono::duration_cast<std::chrono::seconds>(target_time.time_since_epoch()).count() << std::endl;
    
    // Get previous bar values for filling
    auto previous_close = data->get_close(-1);  // Previous bar's close
    auto fill_price = std::isnan(p.fill_price) ? previous_close : p.fill_price;
    auto fill_volume = std::isnan(p.fill_vol) ? 0.0 : p.fill_vol;
    auto fill_oi = std::isnan(p.fill_oi) ? data->get_openinterest(-1) : p.fill_oi;
    
    // Create and insert fill bars
    for (int i = 1; i <= current_bars_to_fill; ++i) {
        auto fill_time = previous_time + (tdunit_ * i);
        
        // Check if fill time is within session
        if (!is_within_session(fill_time)) {
            continue;
        }
        
        // Create fill bar with same OHLC values
        std::vector<double> fill_bar = {
            static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(
                fill_time.time_since_epoch()).count()),  // datetime
            fill_price,     // open
            fill_price,     // high  
            fill_price,     // low
            fill_price,     // close
            fill_volume,    // volume
            fill_oi         // openinterest
        };
        
        // Insert the fill bar into the data source
        data->insert_bar(fill_bar);
        
        std::cout << "Filled bar at " << std::chrono::duration_cast<std::chrono::seconds>(
            fill_time.time_since_epoch()).count() << " with price " << fill_price << std::endl;
    }
}

} // namespace filters
} // namespace backtrader