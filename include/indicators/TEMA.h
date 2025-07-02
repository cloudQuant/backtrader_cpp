#pragma once

#include "IndicatorBase.h"
#include "EMA.h"
#include <memory>

namespace backtrader {

/**
 * @brief Triple Exponential Moving Average (TEMA)
 * 
 * TEMA = 3*EMA1 - 3*EMA2 + EMA3
 * where:
 * EMA1 = EMA(price, period)
 * EMA2 = EMA(EMA1, period)
 * EMA3 = EMA(EMA2, period)
 * 
 * TEMA provides even less lag than DEMA by applying triple smoothing
 */
class TEMA : public IndicatorBase {
private:
    size_t period_;
    std::unique_ptr<EMA> first_ema_;
    std::unique_ptr<EMA> second_ema_;
    std::unique_ptr<EMA> third_ema_;
    std::shared_ptr<LineRoot> ema1_line_;
    std::shared_ptr<LineRoot> ema2_line_;
    
public:
    explicit TEMA(std::shared_ptr<LineRoot> input, size_t period = 21);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    double getEMA1() const { return first_ema_ ? first_ema_->get(0) : NaN; }
    double getEMA2() const { return second_ema_ ? second_ema_->get(0) : NaN; }
    double getEMA3() const { return third_ema_ ? third_ema_->get(0) : NaN; }
};

} // namespace backtrader