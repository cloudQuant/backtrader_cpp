#pragma once

#include "IndicatorBase.h"
#include <memory>

namespace backtrader {

/**
 * @brief On-Balance Volume (OBV) indicator
 * 
 * OBV is a momentum indicator that uses volume flow to predict changes in stock price.
 * It's based on the premise that volume precedes price movement.
 * 
 * Calculation:
 * - If Close > Previous Close: OBV = Previous OBV + Volume
 * - If Close < Previous Close: OBV = Previous OBV - Volume  
 * - If Close = Previous Close: OBV = Previous OBV
 * 
 * OBV is used to confirm price trends and spot potential reversals.
 */
class OBV : public IndicatorBase {
private:
    double current_obv_;
    double prev_close_;
    bool has_prev_close_;
    
public:
    explicit OBV(std::shared_ptr<LineRoot> close_input,
                 std::shared_ptr<LineRoot> volume_input);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    /**
     * @brief Get OBV trend direction
     */
    double getTrendDirection() const;
    
    /**
     * @brief Get OBV momentum
     */
    double getMomentum(size_t lookback = 10) const;
    
    /**
     * @brief Get OBV divergence with price
     */
    double getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback = 20) const;
    
    /**
     * @brief Get OBV slope (rate of change)
     */
    double getSlope(size_t lookback = 5) const;
    
    /**
     * @brief Get volume pressure
     */
    double getVolumePressure() const;
    
    /**
     * @brief Check if OBV is confirming price trend
     */
    bool isConfirmingTrend(std::shared_ptr<LineRoot> price_line, size_t lookback = 10) const;

private:
    void setInputs(std::shared_ptr<LineRoot> close_input,
                   std::shared_ptr<LineRoot> volume_input);
};

} // namespace backtrader