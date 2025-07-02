#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Ultimate Oscillator indicator
 * 
 * The Ultimate Oscillator combines short, intermediate, and long-term price action
 * into one oscillator. It reduces false signals compared to single timeframe oscillators.
 * 
 * UO = 100 * [(4 * Avg7) + (2 * Avg14) + Avg28] / (4 + 2 + 1)
 * 
 * Where:
 * Avg7 = 7-period average of (BP / TR)
 * Avg14 = 14-period average of (BP / TR)  
 * Avg28 = 28-period average of (BP / TR)
 * BP = Buying Pressure = Close - MIN(Low, Previous Close)
 * TR = True Range = MAX(High, Previous Close) - MIN(Low, Previous Close)
 */
class UltimateOscillator : public IndicatorBase {
private:
    size_t period1_;  // Short period (typically 7)
    size_t period2_;  // Medium period (typically 14)
    size_t period3_;  // Long period (typically 28)
    
    CircularBuffer<double> high_buffer_;
    CircularBuffer<double> low_buffer_;
    CircularBuffer<double> close_buffer_;
    CircularBuffer<double> bp_buffer_;   // Buying Pressure
    CircularBuffer<double> tr_buffer_;   // True Range
    
    // Sums for moving averages
    double bp_sum1_, bp_sum2_, bp_sum3_;
    double tr_sum1_, tr_sum2_, tr_sum3_;
    
public:
    explicit UltimateOscillator(std::shared_ptr<LineRoot> high_input,
                               std::shared_ptr<LineRoot> low_input,
                               std::shared_ptr<LineRoot> close_input,
                               size_t period1 = 7,
                               size_t period2 = 14,
                               size_t period3 = 28);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    std::tuple<size_t, size_t, size_t> getPeriods() const { return {period1_, period2_, period3_}; }
    void setPeriods(size_t period1, size_t period2, size_t period3);
    
    /**
     * @brief Get overbought/oversold signal
     */
    double getOverboughtOversold(double overbought_level = 70.0, double oversold_level = 30.0) const;
    
    /**
     * @brief Get bullish/bearish divergence signal
     */
    double getDivergenceSignal() const;
    
    /**
     * @brief Get UO momentum
     */
    double getMomentum(size_t lookback = 3) const;
    
    /**
     * @brief Get buying pressure
     */
    double getBuyingPressure() const;
    
    /**
     * @brief Get true range
     */
    double getTrueRange() const;
    
    /**
     * @brief Get UO trend strength
     */
    double getTrendStrength() const;

private:
    void setInputs(std::shared_ptr<LineRoot> high_input,
                   std::shared_ptr<LineRoot> low_input,
                   std::shared_ptr<LineRoot> close_input);
    
    double calculateBuyingPressure(double close, double low, double prev_close) const;
    double calculateTrueRange(double high, double low, double prev_close) const;
    void updateSums(double bp, double tr);
};

} // namespace backtrader