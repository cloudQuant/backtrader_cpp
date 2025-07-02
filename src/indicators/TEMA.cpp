#include "indicators/TEMA.h"
#include "Common.h"
#include <stdexcept>

namespace backtrader {

TEMA::TEMA(std::shared_ptr<LineRoot> input, size_t period)
    : IndicatorBase(input, "TEMA"),
      period_(period) {
    
    if (period == 0) {
        throw std::invalid_argument("TEMA period must be greater than 0");
    }
    
    setParam("period", static_cast<double>(period));
    setMinPeriod(3 * period - 2);  // TEMA needs 3*period-2 points
    
    // Initialize EMA indicators
    first_ema_ = std::make_unique<EMA>(input, period);
    
    // Create lines for EMA outputs
    ema1_line_ = std::make_shared<LineRoot>(1000, "ema1");
    ema2_line_ = std::make_shared<LineRoot>(1000, "ema2");
    
    second_ema_ = std::make_unique<EMA>(ema1_line_, period);
    third_ema_ = std::make_unique<EMA>(ema2_line_, period);
}

void TEMA::reset() {
    IndicatorBase::reset();
    if (first_ema_) first_ema_->reset();
    if (second_ema_) second_ema_->reset();
    if (third_ema_) third_ema_->reset();
    if (ema1_line_) ema1_line_->home();
    if (ema2_line_) ema2_line_->home();
}

void TEMA::calculate() {
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
    ema1_line_->forward(ema1);
    second_ema_->calculate();
    double ema2 = second_ema_->get(0);
    
    if (isNaN(ema2)) {
        setOutput(0, NaN);
        return;
    }
    
    // Feed second EMA output to third EMA
    ema2_line_->forward(ema2);
    third_ema_->calculate();
    double ema3 = third_ema_->get(0);
    
    if (isNaN(ema3)) {
        setOutput(0, NaN);
        return;
    }
    
    // TEMA formula: 3*EMA1 - 3*EMA2 + EMA3
    double tema = 3.0 * ema1 - 3.0 * ema2 + ema3;
    setOutput(0, tema);
}

void TEMA::calculateBatch(size_t start, size_t end) {
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

void TEMA::setPeriod(size_t period) {
    if (period == 0) {
        throw std::invalid_argument("TEMA period must be greater than 0");
    }
    
    period_ = period;
    setParam("period", static_cast<double>(period));
    setMinPeriod(3 * period - 2);
    
    reset();
}

} // namespace backtrader