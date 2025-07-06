#include "indicators/kamaenvelope.h"
#include <cmath>
#include <limits>

namespace backtrader {
namespace indicators {

KAMAEnvelope::KAMAEnvelope() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

KAMAEnvelope::KAMAEnvelope(std::shared_ptr<LineRoot> data, int period, int fast, int slow, double perc) 
    : Indicator() {
    params.period = period;
    params.fast = fast;
    params.slow = slow;
    params.perc = perc;
    
    setup_lines();
    _minperiod(params.period);
    
    // Create KAMA indicator
    kama_ = std::make_shared<KAMA>(data, period, fast, slow);
}

void KAMAEnvelope::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // mid
        lines->add_line(std::make_shared<LineBuffer>());  // top
        lines->add_line(std::make_shared<LineBuffer>());  // bot
    }
}

double KAMAEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto mid_line = lines->getline(Lines::mid);
    if (!mid_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*mid_line)[ago];
}

int KAMAEnvelope::getMinPeriod() const {
    return params.period;
}

void KAMAEnvelope::calculate() {
    if (kama_) {
        kama_->calculate();
    }
    next();
}

double KAMAEnvelope::getMidLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto mid_line = lines->getline(Lines::mid);
    if (!mid_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*mid_line)[ago];
}

double KAMAEnvelope::getUpperLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto top_line = lines->getline(Lines::top);
    if (!top_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*top_line)[ago];
}

double KAMAEnvelope::getLowerLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto bot_line = lines->getline(Lines::bot);
    if (!bot_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*bot_line)[ago];
}

void KAMAEnvelope::next() {
    if (!kama_) return;
    
    // Get KAMA value
    double kama_value = kama_->get(0);
    
    if (!std::isnan(kama_value)) {
        double perc_factor = params.perc / 100.0;
        
        // Calculate envelope values
        double mid_value = kama_value;
        double upper_value = kama_value * (1.0 + perc_factor);
        double lower_value = kama_value * (1.0 - perc_factor);
        
        // Set line values using the standard pattern
        if (lines && lines->size() > Lines::mid) {
            auto mid_line = lines->getline(Lines::mid);
            if (mid_line) mid_line->set(0, mid_value);
        }
        
        if (lines && lines->size() > Lines::top) {
            auto top_line = lines->getline(Lines::top);
            if (top_line) top_line->set(0, upper_value);
        }
        
        if (lines && lines->size() > Lines::bot) {
            auto bot_line = lines->getline(Lines::bot);
            if (bot_line) bot_line->set(0, lower_value);
        }
    }
}

void KAMAEnvelope::once(int start, int end) {
    for (int i = start; i <= end; ++i) {
        next();
    }
}

} // namespace indicators
} // namespace backtrader