#pragma once

#include "IndicatorBase.h"
#include "CircularBuffer.h"
#include <memory>

namespace backtrader {

/**
 * @brief Stochastic Oscillator indicator
 */
class Stochastic : public IndicatorBase {
private:
    size_t k_period_;
    size_t k_slow_;
    size_t d_period_;
    
    CircularBuffer<double> high_buffer_;
    CircularBuffer<double> low_buffer_;
    CircularBuffer<double> close_buffer_;
    CircularBuffer<double> k_values_;
    
public:
    explicit Stochastic(std::shared_ptr<LineRoot> high_input,
                       std::shared_ptr<LineRoot> low_input,
                       std::shared_ptr<LineRoot> close_input,
                       size_t k_period = 14,
                       size_t k_slow = 3,
                       size_t d_period = 3);
    
    void reset() override;
    void calculate() override;
    void calculateBatch(size_t start, size_t end) override;
    
    double getK() const;
    double getD() const;
    double getOverboughtOversold(double overbought_level = 80.0, double oversold_level = 20.0) const;
    double getCrossoverSignal() const;

private:
    void setInputs(std::shared_ptr<LineRoot> high_input,
                   std::shared_ptr<LineRoot> low_input,
                   std::shared_ptr<LineRoot> close_input);
    
    double calculateRawK(double close, double highest_high, double lowest_low) const;
    double calculateSMA(const CircularBuffer<double>& buffer, size_t period) const;
};

} // namespace backtrader