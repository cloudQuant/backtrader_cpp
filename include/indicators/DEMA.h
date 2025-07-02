#pragma once

#include "IndicatorBase.h"
#include "EMA.h"
#include <memory>

namespace backtrader {

/**
 * @brief Double Exponential Moving Average (DEMA)
 * 
 * DEMA = 2*EMA(price, period) - EMA(EMA(price, period), period)
 * 
 * DEMA provides less lag than standard EMA by applying double smoothing
 * and then compensating for the extra lag.
 */
class DEMA : public IndicatorBase {
private:
    size_t period_;
    std::unique_ptr<EMA> first_ema_;
    std::unique_ptr<EMA> second_ema_;
    std::shared_ptr<LineRoot> ema_line_;
    
public:
    explicit DEMA(std::shared_ptr<LineRoot> input, size_t period = 21);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    double getEMA1() const { return first_ema_ ? first_ema_->get(0) : NaN; }
    double getEMA2() const { return second_ema_ ? second_ema_->get(0) : NaN; }
};

} // namespace backtrader