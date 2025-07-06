#include "indicators/lowest.h"
#include "linebuffer.h"
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
    if (!data_source_ || current_index_ >= data_source_->size()) return;
    
    // Calculate lowest value over period
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