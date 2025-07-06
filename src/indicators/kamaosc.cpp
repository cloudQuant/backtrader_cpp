#include "indicators/kamaosc.h"
#include <cmath>
#include <limits>

namespace backtrader {
namespace indicators {

KAMAOscillator::KAMAOscillator() : Indicator() {
    setup_lines();
    _minperiod(std::max(params.period1, params.period2));
}

KAMAOscillator::KAMAOscillator(std::shared_ptr<LineRoot> data, int period1, int period2, int fast, int slow) 
    : Indicator() {
    params.period1 = period1;
    params.period2 = period2;
    params.fast = fast;
    params.slow = slow;
    
    setup_lines();
    _minperiod(std::max(params.period1, params.period2));
    
    // Create KAMA indicators
    kama1_ = std::make_shared<KAMA>(data, period1, fast, slow);  // Fast KAMA
    kama2_ = std::make_shared<KAMA>(data, period2, fast, slow);  // Slow KAMA
}

void KAMAOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // oscillator line
    }
}

double KAMAOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(0);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int KAMAOscillator::getMinPeriod() const {
    return std::max(params.period1, params.period2);
}

void KAMAOscillator::calculate() {
    if (kama1_) {
        kama1_->calculate();
    }
    if (kama2_) {
        kama2_->calculate();
    }
    next();
}

void KAMAOscillator::next() {
    if (!kama1_ || !kama2_) return;
    
    // Get KAMA values
    double fast_kama = kama1_->get(0);
    double slow_kama = kama2_->get(0);
    
    if (!std::isnan(fast_kama) && !std::isnan(slow_kama)) {
        // Calculate oscillator as difference between fast and slow KAMA
        double osc_value = fast_kama - slow_kama;
        
        if (lines && lines->size() > 0) {
            auto osc_line = lines->getline(0);
            if (osc_line) osc_line->set(0, osc_value);
        }
    }
}

void KAMAOscillator::once(int start, int end) {
    for (int i = start; i <= end; ++i) {
        next();
    }
}

} // namespace indicators
} // namespace backtrader