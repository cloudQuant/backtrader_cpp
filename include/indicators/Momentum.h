#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Momentum indicator
 * 
 * Momentum measures the rate of change in price over a specified period.
 * It can be calculated as either difference or ratio:
 * 
 * Difference Momentum = Current Price - Price N periods ago
 * Ratio Momentum = (Current Price / Price N periods ago) * 100
 * 
 * Momentum oscillates around zero (difference) or 100 (ratio).
 * Positive values indicate upward momentum, negative values indicate downward momentum.
 */
class Momentum : public IndicatorBase {
private:
    size_t period_;
    bool use_ratio_;  // True for ratio, false for difference
    
    CircularBuffer<double> price_buffer_;
    
public:
    explicit Momentum(std::shared_ptr<LineRoot> input,
                     size_t period = 12,
                     bool use_ratio = false);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    bool isUsingRatio() const { return use_ratio_; }
    void setUseRatio(bool use_ratio);
    
    /**
     * @brief Get momentum signal based on zero/centerline crossover
     */
    double getMomentumSignal() const;
    
    /**
     * @brief Get momentum strength (0-100 scale)
     */
    double getMomentumStrength() const;
    
    /**
     * @brief Get momentum acceleration (rate of change of momentum)
     */
    double getAcceleration(size_t lookback = 1) const;
    
    /**
     * @brief Get momentum divergence with price
     */
    double getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback = 5) const;
    
    /**
     * @brief Get momentum trend direction
     */
    double getTrendDirection() const;
    
    /**
     * @brief Get momentum volatility
     */
    double getVolatility(size_t lookback = 10) const;
    
    /**
     * @brief Check if momentum is increasing
     */
    bool isIncreasing(size_t lookback = 3) const;
    
    /**
     * @brief Check if momentum is decreasing
     */
    bool isDecreasing(size_t lookback = 3) const;

private:
    double getCenterline() const;
};

} // namespace backtrader