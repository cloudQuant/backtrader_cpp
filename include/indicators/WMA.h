#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Weighted Moving Average (WMA)
 * 
 * WMA gives more weight to recent prices, with weights decreasing linearly.
 * Most recent price gets weight n, previous gets weight n-1, etc.
 * 
 * WMA = (P1*n + P2*(n-1) + ... + Pn*1) / (n + (n-1) + ... + 1)
 * where n is the period and P1 is the most recent price
 */
class WMA : public IndicatorBase {
private:
    size_t period_;
    CircularBuffer<double> price_buffer_;
    double weight_sum_;
    
public:
    explicit WMA(std::shared_ptr<LineRoot> input, size_t period = 14);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    /**
     * @brief Get slope of WMA (rate of change)
     */
    double getSlope(size_t lookback = 1) const;
    
    /**
     * @brief Get trend strength based on slope consistency
     */
    double getTrendStrength(size_t lookback = 5) const;
    
    /**
     * @brief Get crossover signal with reference line
     */
    double getCrossoverSignal(std::shared_ptr<LineRoot> reference_line) const;
    
    /**
     * @brief Get volatility of WMA values
     */
    double getVolatility(size_t lookback = 10) const;
};

} // namespace backtrader