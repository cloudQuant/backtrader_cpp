#include "../../include/filters/heikinashi.h"
#include <algorithm>

namespace backtrader {
namespace filters {

HeikinAshi::HeikinAshi(const Params& params) : p(params) {}

void HeikinAshi::__call__(std::shared_ptr<DataSeries> data) {
    if (!data || data->lines.empty() || data->lines.size() < 5) {
        return;
    }
    
    auto& datetime_line = data->lines[0];
    auto& open_line = data->lines[1];
    auto& high_line = data->lines[2];
    auto& low_line = data->lines[3];
    auto& close_line = data->lines[4];
    
    if (datetime_line.empty()) {
        return;
    }
    
    // Create new Heikin-Ashi data
    std::vector<double> ha_open, ha_high, ha_low, ha_close;
    ha_open.reserve(datetime_line.size());
    ha_high.reserve(datetime_line.size());
    ha_low.reserve(datetime_line.size());
    ha_close.reserve(datetime_line.size());
    
    for (size_t i = 0; i < datetime_line.size(); ++i) {
        double open = open_line[i];
        double high = high_line[i];
        double low = low_line[i];
        double close = close_line[i];
        
        // Calculate Heikin-Ashi values
        double ha_c = (open + high + low + close) / 4.0;
        
        double ha_o;
        if (i == 0) {
            ha_o = (open + close) / 2.0;
        } else {
            ha_o = (ha_open[i-1] + ha_close[i-1]) / 2.0;
        }
        
        double ha_h = std::max({high, ha_o, ha_c});
        double ha_l = std::min({low, ha_o, ha_c});
        
        ha_open.push_back(ha_o);
        ha_high.push_back(ha_h);
        ha_low.push_back(ha_l);
        ha_close.push_back(ha_c);
    }
    
    // Replace original OHLC data with Heikin-Ashi data
    if (p.openha) {
        open_line = ha_open;
    }
    if (p.highha) {
        high_line = ha_high;
    }
    if (p.lowha) {
        low_line = ha_low;
    }
    if (p.closeha) {
        close_line = ha_close;
    }
}

} // namespace filters
} // namespace backtrader