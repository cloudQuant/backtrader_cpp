#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Kaufman Adaptive Moving Average (KAMA)
 * 
 * KAMA adapts to market volatility by using an Efficiency Ratio to determine
 * the smoothing constant. In trending markets it acts like a fast EMA,
 * in sideways markets it acts like a slow EMA.
 * 
 * ER = Direction / Volatility
 * SC = [ER * (FastSC - SlowSC) + SlowSC]²
 * KAMA = Previous KAMA + SC * (Price - Previous KAMA)
 */
class KAMA : public IndicatorBase {
private:
    size_t period_;
    size_t fast_sc_;  // Fast smoothing constant period
    size_t slow_sc_;  // Slow smoothing constant period
    
    CircularBuffer<double> price_buffer_;
    double prev_kama_;
    bool has_prev_kama_;
    
    // Pre-calculated smoothing constants
    double fast_sc_calc_;
    double slow_sc_calc_;
    
public:
    explicit KAMA(std::shared_ptr<LineRoot> input, 
                  size_t period = 14,
                  size_t fast_sc = 2,
                  size_t slow_sc = 30);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    std::pair<size_t, size_t> getSmoothing() const { return {fast_sc_, slow_sc_}; }
    void setSmoothing(size_t fast_sc, size_t slow_sc);
    
    /**
     * @brief Get current Efficiency Ratio
     */
    double getEfficiencyRatio() const;
    
    /**
     * @brief Get current Smoothing Constant
     */
    double getSmoothingConstant() const;
    
    /**
     * @brief Get trend signal based on KAMA direction
     */
    double getTrendSignal(double threshold = 0.01) const;
    
    /**
     * @brief Get KAMA volatility
     */
    double getVolatility(size_t lookback = 10) const;
    
    /**
     * @brief Get adaptive speed as percentage (0-100%)
     */
    double getAdaptiveSpeed() const;
    
    /**
     * @brief Get crossover signal with reference line
     */
    double getCrossoverSignal(std::shared_ptr<LineRoot> reference_line) const;
};

} // namespace backtrader