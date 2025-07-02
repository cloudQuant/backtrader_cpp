#pragma once

#include "IndicatorBase.h"
#include <memory>

namespace backtrader {

/**
 * @brief TRIX indicator
 * 
 * TRIX is a momentum oscillator that displays the percent rate of change 
 * of a triple exponentially smoothed moving average.
 * 
 * Calculation:
 * 1. First EMA = EMA(Close, period)
 * 2. Second EMA = EMA(First EMA, period)  
 * 3. Third EMA = EMA(Second EMA, period)
 * 4. TRIX = (Third EMA - Previous Third EMA) / Previous Third EMA * 10000
 */
class TRIX : public IndicatorBase {
private:
    size_t period_;
    double multiplier_;
    
    double ema1_;          // First EMA
    double ema2_;          // Second EMA  
    double ema3_;          // Third EMA
    double prev_ema3_;     // Previous Third EMA
    
    bool has_ema1_;
    bool has_ema2_;
    bool has_ema3_;
    bool has_prev_ema3_;
    
public:
    explicit TRIX(std::shared_ptr<LineRoot> input,
                  size_t period = 14);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    /**
     * @brief Get TRIX signal line (EMA of TRIX)
     */
    double getSignalLine(size_t signal_period = 9) const;
    
    /**
     * @brief Get zero-line crossover signal
     */
    double getZeroCrossSignal() const;
    
    /**
     * @brief Get TRIX-Signal crossover signal
     */
    double getSignalCrossover(size_t signal_period = 9) const;
    
    /**
     * @brief Get TRIX momentum
     */
    double getMomentum(size_t lookback = 5) const;
    
    /**
     * @brief Get TRIX divergence with price
     */
    double getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback = 20) const;
    
    /**
     * @brief Get trend direction
     */
    double getTrendDirection() const;
    
    /**
     * @brief Get current triple EMA value
     */
    double getTripleEMA() const { return ema3_; }

private:
    double calculateEMA(double current_value, double prev_ema, bool& has_prev);
};

} // namespace backtrader