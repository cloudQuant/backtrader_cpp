#pragma once

#include "IndicatorBase.h"
#include <memory>
#include <vector>

namespace backtrader {

/**
 * @brief Parabolic Stop and Reverse (SAR) indicator
 * 
 * The Parabolic SAR is a trend-following indicator that provides entry and exit points.
 * It appears as dots above or below the price, indicating the direction of the trend.
 * When the price crosses the SAR, it signals a potential trend reversal.
 * 
 * Calculation:
 * - During uptrend: SAR = Prior SAR + AF * (Prior EP - Prior SAR)
 * - During downtrend: SAR = Prior SAR - AF * (Prior SAR - Prior EP)
 * 
 * Where:
 * - AF (Acceleration Factor) starts at 0.02 and increases by 0.02 each time a new extreme is reached
 * - EP (Extreme Point) is the highest high in uptrend or lowest low in downtrend
 * - Maximum AF is typically 0.20
 */
class ParabolicSAR : public IndicatorBase {
private:
    double af_start_;         // Starting acceleration factor
    double af_increment_;     // AF increment
    double af_maximum_;       // Maximum AF
    
    // Current state
    double current_sar_;      // Current SAR value
    double current_af_;       // Current acceleration factor
    double extreme_point_;    // Current extreme point
    bool is_long_;           // True if in long position (SAR below price)
    bool initialized_;       // True after first calculation
    
    // Price history for initialization
    std::vector<double> high_history_;
    std::vector<double> low_history_;
    
public:
    explicit ParabolicSAR(std::shared_ptr<LineRoot> high_input,
                         std::shared_ptr<LineRoot> low_input,
                         double af_start = 0.02,
                         double af_increment = 0.02,
                         double af_maximum = 0.20);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    // Parameter getters/setters
    double getAFStart() const { return af_start_; }
    double getAFIncrement() const { return af_increment_; }
    double getAFMaximum() const { return af_maximum_; }
    
    void setAFStart(double af_start);
    void setAFIncrement(double af_increment);
    void setAFMaximum(double af_maximum);
    void setAFParameters(double af_start, double af_increment, double af_maximum);
    
    /**
     * @brief Get current acceleration factor
     */
    double getCurrentAF() const { return current_af_; }
    
    /**
     * @brief Get current extreme point
     */
    double getExtremePoint() const { return extreme_point_; }
    
    /**
     * @brief Check if currently in long position (SAR below price)
     */
    bool isLong() const { return is_long_; }
    
    /**
     * @brief Check if currently in short position (SAR above price)
     */
    bool isShort() const { return !is_long_; }
    
    /**
     * @brief Get trend reversal signal
     * Returns 1.0 for bullish reversal, -1.0 for bearish reversal, 0.0 for no signal
     */
    double getReversalSignal(std::shared_ptr<LineRoot> close_input) const;
    
    /**
     * @brief Get trend direction signal
     * Returns 1.0 for uptrend, -1.0 for downtrend, 0.0 for undefined
     */
    double getTrendDirection() const;
    
    /**
     * @brief Get stop loss level for current position
     */
    double getStopLoss() const;
    
    /**
     * @brief Get distance from price to SAR as percentage
     */
    double getDistanceToSAR(double current_price) const;
    
    /**
     * @brief Check if SAR is accelerating (AF increasing)
     */
    bool isAccelerating() const;
    
    /**
     * @brief Get SAR momentum (rate of change)
     */
    double getSARMomentum(size_t lookback = 3) const;

private:
    void setInputs(std::shared_ptr<LineRoot> high_input, std::shared_ptr<LineRoot> low_input);
    void initialize(double high, double low);
    void updateSAR(double high, double low);
    bool checkReversal(double high, double low) const;
    void reversePosition(double high, double low);
    void updateExtremePoint(double high, double low);
};

} // namespace backtrader