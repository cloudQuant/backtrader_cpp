#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Average Directional Index (ADX) indicator
 * 
 * ADX measures trend strength without regard to trend direction.
 * Values range from 0 to 100, with higher values indicating stronger trends.
 * 
 * Calculation:
 * 1. Calculate True Range (TR)
 * 2. Calculate +DM (Positive Directional Movement) and -DM (Negative Directional Movement)
 * 3. Calculate +DI (Positive Directional Indicator) and -DI (Negative Directional Indicator)
 * 4. Calculate DX (Directional Index) = |+DI - -DI| / (+DI + -DI) * 100
 * 5. Calculate ADX as smoothed average of DX
 * 
 * Interpretation:
 * - ADX < 20: Weak trend (sideways market)
 * - ADX 20-40: Moderate trend
 * - ADX > 40: Strong trend
 * - ADX > 60: Very strong trend
 */
class ADX : public IndicatorBase {
private:
    size_t period_;
    
    // Circular buffers for OHLC data
    CircularBuffer<double> high_buffer_;
    CircularBuffer<double> low_buffer_;
    CircularBuffer<double> close_buffer_;
    
    // Smoothed values
    double smoothed_tr_;
    double smoothed_plus_dm_;
    double smoothed_minus_dm_;
    double smoothed_dx_;
    
    // Initialization flags
    bool has_smoothed_values_;
    int calculation_count_;
    
    // DX history for ADX calculation
    CircularBuffer<double> dx_buffer_;
    
public:
    explicit ADX(std::shared_ptr<LineRoot> high_input,
                 std::shared_ptr<LineRoot> low_input,
                 std::shared_ptr<LineRoot> close_input,
                 size_t period = 14);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    /**
     * @brief Get current +DI (Positive Directional Indicator)
     */
    double getPlusDI() const;
    
    /**
     * @brief Get current -DI (Negative Directional Indicator)
     */
    double getMinusDI() const;
    
    /**
     * @brief Get current DX (Directional Index)
     */
    double getDX() const;
    
    /**
     * @brief Get trend strength signal
     */
    double getTrendStrength() const;
    
    /**
     * @brief Get trend direction signal based on +DI and -DI
     */
    double getTrendDirection() const;
    
    /**
     * @brief Get crossover signal between +DI and -DI
     */
    double getDICrossover() const;
    
    /**
     * @brief Check if market is trending (ADX above threshold)
     */
    bool isTrending(double threshold = 25.0) const;
    
    /**
     * @brief Check if trend is strengthening
     */
    bool isTrendStrengthening() const;
    
    /**
     * @brief Get ADX slope (rate of change)
     */
    double getADXSlope(size_t lookback = 3) const;
    
    /**
     * @brief Get volatility based on True Range
     */
    double getVolatility() const;

private:
    void setInputs(std::shared_ptr<LineRoot> high_input,
                   std::shared_ptr<LineRoot> low_input,
                   std::shared_ptr<LineRoot> close_input);
    
    double calculateTrueRange(double high, double low, double prev_close) const;
    std::pair<double, double> calculateDirectionalMovement(double high, double low, 
                                                          double prev_high, double prev_low) const;
    double wilderSmoothing(double current_value, double previous_smoothed, size_t period) const;
};

} // namespace backtrader