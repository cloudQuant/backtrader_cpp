#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Money Flow Index (MFI) indicator
 * 
 * MFI is a momentum oscillator that uses both price and volume to identify
 * overbought or oversold conditions. It's often called "Volume-weighted RSI".
 * 
 * MFI = 100 - (100 / (1 + Money Flow Ratio))
 * Money Flow Ratio = Positive Money Flow / Negative Money Flow
 * 
 * Raw Money Flow = Typical Price * Volume
 * Positive Money Flow = sum of positive Raw Money Flow over period
 * Negative Money Flow = sum of negative Raw Money Flow over period
 */
class MFI : public IndicatorBase {
private:
    size_t period_;
    
    CircularBuffer<double> high_buffer_;
    CircularBuffer<double> low_buffer_;
    CircularBuffer<double> close_buffer_;
    CircularBuffer<double> volume_buffer_;
    CircularBuffer<double> tp_buffer_;        // Typical Price
    CircularBuffer<double> raw_mf_buffer_;    // Raw Money Flow
    CircularBuffer<int> mf_direction_buffer_; // 1 for positive, -1 for negative, 0 for unchanged
    
    double positive_mf_sum_;
    double negative_mf_sum_;
    
public:
    explicit MFI(std::shared_ptr<LineRoot> high_input,
                 std::shared_ptr<LineRoot> low_input,
                 std::shared_ptr<LineRoot> close_input,
                 std::shared_ptr<LineRoot> volume_input,
                 size_t period = 14);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    /**
     * @brief Get overbought/oversold signal
     */
    double getOverboughtOversold(double overbought_level = 80.0, double oversold_level = 20.0) const;
    
    /**
     * @brief Get MFI divergence with price
     */
    double getDivergence(std::shared_ptr<LineRoot> price_line, size_t lookback = 5) const;
    
    /**
     * @brief Get money flow ratio
     */
    double getMoneyFlowRatio() const;
    
    /**
     * @brief Get positive money flow
     */
    double getPositiveMoneyFlow() const { return positive_mf_sum_; }
    
    /**
     * @brief Get negative money flow
     */
    double getNegativeMoneyFlow() const { return negative_mf_sum_; }
    
    /**
     * @brief Get current raw money flow
     */
    double getRawMoneyFlow() const;
    
    /**
     * @brief Get MFI momentum
     */
    double getMomentum(size_t lookback = 3) const;
    
    /**
     * @brief Check if MFI is in extreme territory
     */
    bool isExtreme(double extreme_high = 90.0, double extreme_low = 10.0) const;
    
    /**
     * @brief Get MFI trend strength
     */
    double getTrendStrength() const;

private:
    void setInputs(std::shared_ptr<LineRoot> high_input,
                   std::shared_ptr<LineRoot> low_input,
                   std::shared_ptr<LineRoot> close_input,
                   std::shared_ptr<LineRoot> volume_input);
    
    double calculateTypicalPrice(double high, double low, double close) const;
    void updateMoneyFlowSums(double raw_mf, int direction);
};

} // namespace backtrader