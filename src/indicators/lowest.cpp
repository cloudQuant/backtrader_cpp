#include "indicators/lowest.h"
#include "linebuffer.h"
#include "lineroot.h"
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

Lowest::Lowest() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = 14;
    setup_lines();
}

Lowest::Lowest(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
}

Lowest::Lowest(std::shared_ptr<LineRoot> data_source, int period)
    : Indicator(), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(period);
    
    // Try to cast LineRoot to LineSeries, or work with LineRoot directly
    data_source_ = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (!data_source_) {
        // If it's not a LineSeries, we'll need to work differently
        // For now, store the LineRoot and handle it in calculate()
        root_data_source_ = data_source;
    }
}

void Lowest::setup_lines() {
    if (!lines) {
        lines = std::make_shared<Lines>();
        lines->add_line(std::make_shared<LineBuffer>());
    }
    lines->add_alias("lowest", 0);
}

double Lowest::get(int ago) const {
    if (!lines || ago < 0) return 0.0;
    auto line = lines->getline(0);
    if (!line) return 0.0;
    return (*line)[ago];
}

int Lowest::getMinPeriod() const {
    return params.period;
}

void Lowest::calculate() {
    // Handle both LineSeries and LineRoot data sources
    if (data_source_) {
        if (current_index_ >= data_source_->size()) return;
        
        // Calculate lowest value over period using LineSeries
        double lowest_val = std::numeric_limits<double>::max();
        size_t start = (current_index_ >= params.period - 1) ? current_index_ - params.period + 1 : 0;
        
        for (size_t i = start; i <= current_index_; ++i) {
            double val = (*data_source_)[i];
            if (val < lowest_val) {
                lowest_val = val;
            }
        }
        
        // Set the calculated value
        if (lines) {
            auto lowest_line = lines->getline(0);
            if (lowest_line) lowest_line->set(0, lowest_val);
        }
    } else if (root_data_source_) {
        // Handle LineRoot - try to cast to LineSingle for data access
        auto single_line = std::dynamic_pointer_cast<LineSingle>(root_data_source_);
        if (single_line) {
            double lowest_val = std::numeric_limits<double>::max();
            size_t data_size = single_line->size();
            int start_ago = std::min(params.period - 1, static_cast<int>(data_size) - 1);
            
            for (int ago = start_ago; ago >= 0; --ago) {
                double val = (*single_line)[ago];
                if (val < lowest_val) {
                    lowest_val = val;
                }
            }
            
            // Set the calculated value
            if (lines) {
                auto lowest_line = lines->getline(0);
                if (lowest_line) lowest_line->set(0, lowest_val);
            }
        }
    }
    
    current_index_++;
}

void Lowest::next() {
    calculate();
}

void Lowest::once(int start, int end) {
    for (int i = start; i < end; ++i) {
        current_index_ = i;
        calculate();
    }
}

} // namespace indicators
} // namespace backtrader