#include "indicators/williamsr.h"
#include <cmath>
#include <algorithm>

namespace backtrader {
namespace indicators {

WilliamsR::WilliamsR(int period) : Indicator(), using_line_roots_(false) {
    params.period = period;
    
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("percR", 0);
    }
}

WilliamsR::WilliamsR(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), using_line_roots_(false) {
    params.period = period;
    
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("percR", 0);
    }
    
    // Set data member for compatibility
    data = data_source_;
}

WilliamsR::WilliamsR(std::shared_ptr<LineRoot> close_line, 
                     std::shared_ptr<LineRoot> high_line, 
                     std::shared_ptr<LineRoot> low_line, 
                     int period) 
    : Indicator(), close_line_(close_line), high_line_(high_line), 
      low_line_(low_line), using_line_roots_(true) {
    params.period = period;
    
    // Set minimum period
    _minperiod(period);
    
    // Initialize lines
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("percR", 0);
    }
}

std::vector<std::string> WilliamsR::_get_line_names() const {
    return {"percR"};
}

double WilliamsR::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return lines->getline(0)->get(ago);
}

double WilliamsR::get_highest(int period, int start_ago) {
    if (using_line_roots_) {
        if (!high_line_) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        // Cast to LineSingle to access data
        auto high_line_single = std::dynamic_pointer_cast<LineSingle>(high_line_);
        if (!high_line_single) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        double highest = std::numeric_limits<double>::lowest();
        for (int i = start_ago; i < start_ago + period; ++i) {
            double high_val = high_line_single->get(i);
            if (!std::isnan(high_val) && high_val > highest) {
                highest = high_val;
            }
        }
        return highest;
    } else {
        if (datas.empty() || !datas[0]) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        double highest = std::numeric_limits<double>::lowest();
        for (int i = start_ago; i < start_ago + period; ++i) {
            double high_val = datas[0]->high(i);
            if (!std::isnan(high_val) && high_val > highest) {
                highest = high_val;
            }
        }
        return highest;
    }
}

double WilliamsR::get_lowest(int period, int start_ago) {
    if (using_line_roots_) {
        if (!low_line_) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        // Cast to LineSingle to access data
        auto low_line_single = std::dynamic_pointer_cast<LineSingle>(low_line_);
        if (!low_line_single) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        double lowest = std::numeric_limits<double>::max();
        for (int i = start_ago; i < start_ago + period; ++i) {
            double low_val = low_line_single->get(i);
            if (!std::isnan(low_val) && low_val < lowest) {
                lowest = low_val;
            }
        }
        return lowest;
    } else {
        if (datas.empty() || !datas[0]) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        double lowest = std::numeric_limits<double>::max();
        for (int i = start_ago; i < start_ago + period; ++i) {
            double low_val = datas[0]->low(i);
            if (!std::isnan(low_val) && low_val < lowest) {
                lowest = low_val;
            }
        }
        return lowest;
    }
}

void WilliamsR::next() {
    if (!lines || lines->size() == 0) {
        return;
    }
    
    // Get current close price
    double current_close;
    if (using_line_roots_) {
        if (!close_line_) {
            return;
        }
        
        // Cast to LineSingle to access data
        auto close_line_single = std::dynamic_pointer_cast<LineSingle>(close_line_);
        if (!close_line_single) {
            return;
        }
        current_close = close_line_single->get(0);
    } else {
        if (datas.empty() || !datas[0]) {
            return;
        }
        current_close = datas[0]->close(0);
    }
    
    if (std::isnan(current_close)) {
        auto line = lines->getline(0);
        if (line) {
            line->set(0, std::numeric_limits<double>::quiet_NaN());
        }
        return;
    }
    
    // Calculate highest high and lowest low over the period
    double highest_high = get_highest(params.period, 0);
    double lowest_low = get_lowest(params.period, 0);
    
    // Calculate Williams %R
    double williams_r;
    if (highest_high == lowest_low) {
        williams_r = 0.0;  // Avoid division by zero
    } else {
        williams_r = ((highest_high - current_close) / (highest_high - lowest_low)) * -100.0;
    }
    
    auto line = lines->getline(0);
    if (line) {
        line->set(0, williams_r);
    }
}

void WilliamsR::calculate() {
    next();
}

void WilliamsR::once(int start, int end) {
    // For vectorized computation - implement if needed
    for (int i = start; i < end; ++i) {
        next();
    }
}

} // namespace indicators
} // namespace backtrader