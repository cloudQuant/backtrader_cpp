#pragma once

#include "../metabase.h"
#include "../feed.h"
#include <chrono>
#include <cmath>

namespace backtrader {
namespace filters {

/**
 * CalendarDays - Bar filler to add missing calendar days to trading days
 * 
 * If the data has a gap larger than 1 day amongst bars, the missing bars
 * are added to the stream.
 * 
 * Params:
 *   - fill_price: Price filling strategy
 *     > 0: Use the given value
 *     0 or None: Use the last known closing price
 *     -1: Use the midpoint of the last bar (High-Low average)
 *   - fill_vol: Value to use for missing volume
 *   - fill_oi: Value to use for missing Open Interest
 */
class CalendarDays : public MetaParams {
public:
    // Parameters structure
    struct Params {
        double fill_price = 0.0;  // 0 = use last close, -1 = use midpoint, >0 = use value
        double fill_vol = std::numeric_limits<double>::quiet_NaN();
        double fill_oi = std::numeric_limits<double>::quiet_NaN();
    };

    CalendarDays(std::shared_ptr<AbstractDataBase> data, const Params& params = Params{});
    virtual ~CalendarDays() = default;

    // Filter interface
    bool operator()(std::shared_ptr<AbstractDataBase> data);

private:
    // Parameters
    Params params_;
    
    // One day duration
    static const std::chrono::hours ONE_DAY;
    
    // Last seen date
    std::chrono::system_clock::time_point last_date_;
    
    // Internal methods
    void fill_bars(std::shared_ptr<AbstractDataBase> data, 
                   const std::chrono::system_clock::time_point& current_date,
                   const std::chrono::system_clock::time_point& last_date);
    
    double get_fill_price(std::shared_ptr<AbstractDataBase> data) const;
    
    // Time utility methods
    std::chrono::system_clock::time_point get_date_only(
        const std::chrono::system_clock::time_point& datetime) const;
    
    std::chrono::system_clock::time_point get_time_only(
        const std::chrono::system_clock::time_point& datetime) const;
    
    // Bar creation
    std::vector<double> create_fill_bar(std::shared_ptr<AbstractDataBase> data,
                                       const std::chrono::system_clock::time_point& datetime) const;
};

} // namespace filters
} // namespace backtrader