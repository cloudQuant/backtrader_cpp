#pragma once

#include "../metabase.h"
#include "../feed.h"
#include <chrono>
#include <memory>
#include <limits>

namespace backtrader {
namespace filters {

/**
 * SessionFiller - Bar Filler for a Data Source inside the declared session start/end times
 * 
 * The fill bars are constructed using the declared Data Source timeframe
 * and compression (used to calculate the intervening missing times)
 * 
 * Params:
 *   - fill_price (default: NaN): If NaN, the closing price of the previous bar will be used
 *   - fill_vol (default: NaN): Value to use to fill the missing volume
 *   - fill_oi (default: NaN): Value to use to fill the missing Open Interest
 *   - skip_first_fill (default: true): Upon seeing the 1st valid bar do not fill from sessionstart
 */
class SessionFiller {
public:
    // Parameters structure  
    struct Params {
        double fill_price = std::numeric_limits<double>::quiet_NaN();
        double fill_vol = std::numeric_limits<double>::quiet_NaN();
        double fill_oi = std::numeric_limits<double>::quiet_NaN();
        bool skip_first_fill = true;
    };
    
    SessionFiller(std::shared_ptr<AbstractDataBase> data, const Params& params = Params{});
    virtual ~SessionFiller() = default;
    
    // Main processing function
    bool operator()(std::shared_ptr<AbstractDataBase> data);
    
protected:
    Params p;  // Parameters
    
    // Time calculations
    std::chrono::milliseconds tdframe_;  // Minimum delta unit between bars
    std::chrono::milliseconds tdunit_;   // Time unit based on compression
    
    // State tracking
    bool seenbar_ = false;  // Control if at least one bar has been seen
    std::chrono::system_clock::time_point sessend_;  // Session end control
    
    // Maximum date constant
    static const std::chrono::system_clock::time_point MAXDATE;
    
private:
    // Helper methods
    std::chrono::milliseconds calculate_timeframe_delta(TimeFrame::Value timeframe) const;
    void calculate_session_limits(std::shared_ptr<AbstractDataBase> data);
    bool is_within_session(std::chrono::system_clock::time_point bar_time) const;
    void fill_bars_up_to(std::chrono::system_clock::time_point target_time, 
                        std::shared_ptr<AbstractDataBase> data);
};

} // namespace filters
} // namespace backtrader