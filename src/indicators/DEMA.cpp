#include "indicators/DEMA.h"
#include "Common.h"
#include <stdexcept>

namespace backtrader {

DEMA::DEMA(std::shared_ptr<LineRoot> input, size_t period)
    : IndicatorBase(input, "DEMA"),
      period_(period) {
    
    if (period == 0) {
        throw std::invalid_argument("DEMA period must be greater than 0");
    }
    
    setParam("period", static_cast<double>(period));
    setMinPeriod(2 * period - 1);  // DEMA needs 2*period-1 points
    
    // Initialize EMA indicators
    first_ema_ = std::make_unique<EMA>(input, period);
    
    // Create line for first EMA output to feed into second EMA
    ema_line_ = std::make_shared<LineRoot>(1000, "ema1");
    second_ema_ = std::make_unique<EMA>(ema_line_, period);
}

void DEMA::reset() {
    IndicatorBase::reset();
    if (first_ema_) first_ema_->reset();
    if (second_ema_) second_ema_->reset();
    if (ema_line_) ema_line_->home();
}

void DEMA::calculate() {
    if (!hasValidInput()) {
        setOutput(0, NaN);
        return;
    }
    
    // Calculate first EMA
    first_ema_->calculate();
    double ema1 = first_ema_->get(0);
    
    if (isNaN(ema1)) {
        setOutput(0, NaN);
        return;
    }
    
    // Feed first EMA output to second EMA
    ema_line_->forward(ema1);
    second_ema_->calculate();
    double ema2 = second_ema_->get(0);
    
    if (isNaN(ema2)) {
        setOutput(0, NaN);
        return;
    }
    
    // DEMA formula: 2*EMA1 - EMA2
    double dema = 2.0 * ema1 - ema2;
    setOutput(0, dema);
}

void DEMA::calculateBatch(size_t start, size_t end) {
    if (!hasValidInput()) {
        return;
    }
    
    auto input = getInput(0);
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            input->forward();
        }
    }
}

void DEMA::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("DEMA period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(2 * period - 1);
    
    reset();
}

} // namespace backtrader