#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Rate of Change (ROC) indicator
 * 
 * ROC measures the percentage change or ratio change in price over a specified period.
 * It's a momentum oscillator that fluctuates around zero (for percentage) or 1.0 (for ratio).
 * 
 * Percentage ROC = ((Current - Past) / Past) * 100
 * Ratio ROC = Current / Past
 */
class ROC : public IndicatorBase {
private:
    size_t period_;
    bool as_percentage_;
    CircularBuffer<double> price_buffer_;
    
public:
    explicit ROC(std::shared_ptr<LineRoot> input, 
                 size_t period = 14,
                 bool as_percentage = true);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    bool isAsPercentage() const { return as_percentage_; }
    void setAsPercentage(bool as_percentage);
    
    /**
     * @brief Get trend signal based on ROC magnitude
     */
    double getTrendSignal(double threshold = 5.0) const;
    
    /**
     * @brief Get zero-line crossover signal
     */
    double getZeroCrossSignal() const;
    
    /**
     * @brief Calculate divergence with price
     */
    double getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback = 5) const;
    
    /**
     * @brief Get acceleration (rate of change of ROC)
     */
    double getAcceleration(size_t lookback = 1) const;
    
    /**
     * @brief Get ROC volatility
     */
    double getVolatility(size_t lookback = 10) const;
    
    /**
     * @brief Get absolute ROC strength
     */
    double getStrength() const;
    
    /**
     * @brief Get oscillator position (overbought/oversold)
     */
    double getOscillatorPosition(double upper_threshold = 10.0, double lower_threshold = -10.0) const;
};

} // namespace backtrader