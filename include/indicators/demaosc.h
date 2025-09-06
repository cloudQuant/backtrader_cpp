#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Double Exponential Moving Average Oscillator
class DoubleExponentialMovingAverageOscillator : public Indicator {
public:
    struct Params {
        int period = 30;   // DEMA period for oscillator calculation
    } params;
    
    // Lines
    enum Lines { 
        demaosc = 0  // DEMA Oscillator line
    };
    
    DoubleExponentialMovingAverageOscillator();
    DoubleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source);
    DoubleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period);
    DoubleExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source);
    DoubleExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~DoubleExponentialMovingAverageOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // DEMA calculation values
    double ema1_, ema2_;  // First and second EMA values for DEMA
    
    // Smoothing factors
    double alpha_, alpha1_;
    
    bool first_run_;
};

// Aliases
using DEMAOsc = DoubleExponentialMovingAverageOscillator;
using DEMAOSC = DoubleExponentialMovingAverageOscillator;
using DEMAOscillator = DoubleExponentialMovingAverageOscillator;

} // namespace indicators
} // namespace backtrader