#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief True Strength Index (TSI) indicator
 * 
 * TSI is a momentum oscillator that uses moving averages of price changes
 * to filter out price noise and provide clearer signals.
 * 
 * TSI = 100 * (Double Smoothed PC) / (Double Smoothed Absolute PC)
 * 
 * Where:
 * PC = Price Change = Current Close - Previous Close
 * Double Smoothed PC = EMA(EMA(PC, r), s)
 * Double Smoothed |PC| = EMA(EMA(|PC|, r), s)
 * 
 * Typical parameters: r=25, s=13
 */
class TSI : public IndicatorBase {
private:
    size_t first_smoothing_;   // First smoothing period (r)
    size_t second_smoothing_;  // Second smoothing period (s)
    
    CircularBuffer<double> price_buffer_;
    
    // For double-smoothed momentum
    double momentum_ema1_;      // First EMA of momentum
    double momentum_ema2_;      // Second EMA of momentum (final)
    double abs_momentum_ema1_;  // First EMA of absolute momentum
    double abs_momentum_ema2_;  // Second EMA of absolute momentum (final)
    
    bool has_momentum_ema1_;
    bool has_momentum_ema2_;
    bool has_abs_momentum_ema1_;
    bool has_abs_momentum_ema2_;
    
    // EMA multipliers
    double momentum_multiplier1_;
    double momentum_multiplier2_;
    double abs_momentum_multiplier1_;
    double abs_momentum_multiplier2_;
    
public:
    explicit TSI(std::shared_ptr<LineRoot> input,
                 size_t first_smoothing = 25,
                 size_t second_smoothing = 13);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    std::pair<size_t, size_t> getSmoothingPeriods() const { return {first_smoothing_, second_smoothing_}; }
    void setSmoothingPeriods(size_t first_smoothing, size_t second_smoothing);
    
    /**
     * @brief Get TSI signal line (EMA of TSI)
     */
    double getSignalLine(size_t signal_period = 7) const;
    
    /**
     * @brief Get overbought/oversold signal
     */
    double getOverboughtOversold(double overbought_level = 25.0, double oversold_level = -25.0) const;
    
    /**
     * @brief Get zero-line crossover signal
     */
    double getZeroCrossSignal() const;
    
    /**
     * @brief Get TSI-Signal line crossover
     */
    double getSignalCrossover(size_t signal_period = 7) const;
    
    /**
     * @brief Get TSI momentum
     */
    double getMomentum(size_t lookback = 5) const;
    
    /**
     * @brief Get TSI divergence with price
     */
    double getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback = 20) const;
    
    /**
     * @brief Get trend strength based on TSI
     */
    double getTrendStrength() const;
    
    /**
     * @brief Get current price momentum (raw)
     */
    double getPriceMomentum() const;

private:
    void updateEMAs(double momentum, double abs_momentum);
    double calculateEMA(double current_value, double prev_ema, double multiplier, bool& has_prev);
};

} // namespace backtrader