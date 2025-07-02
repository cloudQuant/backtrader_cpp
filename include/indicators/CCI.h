#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Commodity Channel Index (CCI) indicator
 * 
 * CCI measures the variation of a security's price from its statistical mean.
 * It oscillates around zero, with values typically ranging from -100 to +100.
 * 
 * CCI = (Typical Price - SMA) / (0.015 * Mean Deviation)
 * Typical Price = (High + Low + Close) / 3
 * Mean Deviation = Average of absolute deviations from SMA
 */
class CCI : public IndicatorBase {
private:
    size_t period_;
    double factor_;  // Typically 0.015
    
    CircularBuffer<double> high_buffer_;
    CircularBuffer<double> low_buffer_;
    CircularBuffer<double> close_buffer_;
    CircularBuffer<double> tp_buffer_;  // Typical Price buffer
    
public:
    explicit CCI(std::shared_ptr<LineRoot> high_input,
                 std::shared_ptr<LineRoot> low_input,
                 std::shared_ptr<LineRoot> close_input,
                 size_t period = 20,
                 double factor = 0.015);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    double getFactor() const { return factor_; }
    void setFactor(double factor);
    
    /**
     * @brief Get overbought/oversold signal
     */
    double getOverboughtOversold(double overbought_level = 100.0, double oversold_level = -100.0) const;
    
    /**
     * @brief Get zero-line crossover signal
     */
    double getZeroCrossSignal() const;
    
    /**
     * @brief Get extreme levels signal
     */
    double getExtremeLevels(double extreme_high = 200.0, double extreme_low = -200.0) const;
    
    /**
     * @brief Get CCI momentum
     */
    double getMomentum(size_t lookback = 3) const;
    
    /**
     * @brief Get CCI divergence with price
     */
    double getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback = 5) const;

private:
    void setInputs(std::shared_ptr<LineRoot> high_input,
                   std::shared_ptr<LineRoot> low_input,
                   std::shared_ptr<LineRoot> close_input);
    
    double calculateTypicalPrice(double high, double low, double close) const;
    double calculateMeanDeviation(double sma_tp) const;
};

} // namespace backtrader