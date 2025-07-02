#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Aroon indicator
 * 
 * Aroon measures the time since the highest high and lowest low within a period.
 * It consists of two lines: Aroon Up and Aroon Down.
 * 
 * Aroon Up = ((period - periods since highest high) / period) * 100
 * Aroon Down = ((period - periods since lowest low) / period) * 100
 * Aroon Oscillator = Aroon Up - Aroon Down
 */
class Aroon : public IndicatorBase {
private:
    size_t period_;
    
    CircularBuffer<double> high_buffer_;
    CircularBuffer<double> low_buffer_;
    
public:
    explicit Aroon(std::shared_ptr<LineRoot> high_input,
                   std::shared_ptr<LineRoot> low_input,
                   size_t period = 25);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    size_t getPeriod() const { return period_; }
    void setPeriod(size_t period);
    
    /**
     * @brief Get Aroon Up value
     */
    double getAroonUp() const;
    
    /**
     * @brief Get Aroon Down value
     */
    double getAroonDown() const;
    
    /**
     * @brief Get Aroon Oscillator (AroonUp - AroonDown)
     */
    double getAroonOscillator() const;
    
    /**
     * @brief Get trend signal based on Aroon lines
     */
    double getTrendSignal() const;
    
    /**
     * @brief Get Aroon crossover signal
     */
    double getCrossoverSignal() const;
    
    /**
     * @brief Get trend strength
     */
    double getTrendStrength() const;

private:
    void setInputs(std::shared_ptr<LineRoot> high_input,
                   std::shared_ptr<LineRoot> low_input);
    
    std::pair<int, int> findHighLowPositions() const;
};

} // namespace backtrader