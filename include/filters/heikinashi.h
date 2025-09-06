#pragma once

#include "../feed.h"
#include <algorithm>
#include <memory>

namespace backtrader {
namespace filters {

/**
 * HeikinAshi - Heikin Ashi candlestick filter
 * 
 * The filter remodels the open, high, low, close to make HeikinAshi candlesticks.
 * 
 * Heikin Ashi calculation:
 * - HA_Close = (Open + High + Low + Close) / 4
 * - HA_Open = (Previous_HA_Open + Previous_HA_Close) / 2
 * - HA_High = max(HA_Open, HA_Close, High)
 * - HA_Low = min(HA_Open, HA_Close, Low)
 * 
 * References:
 * - https://en.wikipedia.org/wiki/Candlestick_chart#Heikin_Ashi_candlesticks
 * - http://stockcharts.com/school/doku.php?id=chart_school:chart_analysis:heikin_ashi
 */
class HeikinAshi {
public:
    HeikinAshi(std::shared_ptr<AbstractDataBase> data);
    virtual ~HeikinAshi() = default;

    // Filter interface
    bool operator()(std::shared_ptr<AbstractDataBase> data);

private:
    // Calculate Heikin Ashi values
    void calculate_heikin_ashi(std::shared_ptr<AbstractDataBase> data);
    
    // Get previous Heikin Ashi values if available
    std::pair<double, double> get_previous_ha_values(std::shared_ptr<AbstractDataBase> data) const;
};

} // namespace filters
} // namespace backtrader