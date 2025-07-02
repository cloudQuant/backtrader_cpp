#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Ichimoku Cloud (Ichimoku Kinko Hyo) indicator
 * 
 * The Ichimoku Cloud is a comprehensive indicator that defines support and resistance,
 * identifies trend direction, gauges momentum and provides trading signals.
 * 
 * Components:
 * - Tenkan-sen (Conversion Line): (9-period high + 9-period low) / 2
 * - Kijun-sen (Base Line): (26-period high + 26-period low) / 2  
 * - Senkou Span A (Leading Span A): (Tenkan-sen + Kijun-sen) / 2, projected 26 periods ahead
 * - Senkou Span B (Leading Span B): (52-period high + 52-period low) / 2, projected 26 periods ahead
 * - Chikou Span (Lagging Span): Close projected 26 periods back
 */
class Ichimoku : public IndicatorBase {
private:
    size_t tenkan_period_;
    size_t kijun_period_;
    size_t senkou_b_period_;
    size_t displacement_;
    
    CircularBuffer<double> high_buffer_tenkan_;
    CircularBuffer<double> low_buffer_tenkan_;
    CircularBuffer<double> high_buffer_kijun_;
    CircularBuffer<double> low_buffer_kijun_;
    CircularBuffer<double> high_buffer_senkou_b_;
    CircularBuffer<double> low_buffer_senkou_b_;
    CircularBuffer<double> close_buffer_;
    
public:
    explicit Ichimoku(std::shared_ptr<LineRoot> high_input,
                      std::shared_ptr<LineRoot> low_input,
                      std::shared_ptr<LineRoot> close_input,
                      size_t tenkan_period = 9,
                      size_t kijun_period = 26,
                      size_t senkou_b_period = 52,
                      size_t displacement = 26);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    /**
     * @brief Get Tenkan-sen (Conversion Line)
     */
    double getTenkanSen() const;
    
    /**
     * @brief Get Kijun-sen (Base Line)  
     */
    double getKijunSen() const;
    
    /**
     * @brief Get Senkou Span A (Leading Span A)
     */
    double getSenkouSpanA() const;
    
    /**
     * @brief Get Senkou Span B (Leading Span B)
     */
    double getSenkouSpanB() const;
    
    /**
     * @brief Get Chikou Span (Lagging Span)
     */
    double getChikouSpan() const;
    
    /**
     * @brief Get cloud top (max of Senkou spans)
     */
    double getCloudTop() const;
    
    /**
     * @brief Get cloud bottom (min of Senkou spans)
     */
    double getCloudBottom() const;
    
    /**
     * @brief Get cloud thickness
     */
    double getCloudThickness() const;
    
    /**
     * @brief Get overall trend signal
     */
    double getTrendSignal() const;
    
    /**
     * @brief Get TK cross signal (Tenkan-Kijun crossover)
     */
    double getTKCrossSignal() const;
    
    /**
     * @brief Get price vs cloud signal
     */
    double getPriceCloudSignal() const;
    
    /**
     * @brief Get Chikou confirmation signal
     */
    double getChikouSignal() const;
    
    /**
     * @brief Check if in bullish cloud
     */
    bool isBullishCloud() const;
    
    /**
     * @brief Check if price is above cloud
     */
    bool isPriceAboveCloud() const;

private:
    void setInputs(std::shared_ptr<LineRoot> high_input,
                   std::shared_ptr<LineRoot> low_input,
                   std::shared_ptr<LineRoot> close_input);
    
    double calculateMidpoint(const CircularBuffer<double>& high_buffer,
                           const CircularBuffer<double>& low_buffer) const;
};

} // namespace backtrader