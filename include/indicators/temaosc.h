#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Triple Exponential Moving Average Oscillator
class TripleExponentialMovingAverageOscillator : public Indicator {
public:
    struct Params {
        int period1 = 12;   // Fast TEMA period
        int period2 = 26;   // Slow TEMA period
    } params;
    
    // Lines
    enum Lines { 
        temaosc = 0  // TEMA Oscillator line
    };
    
    TripleExponentialMovingAverageOscillator();
    TripleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source);
    TripleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period1, int period2);
    // Constructor for test framework compatibility
    TripleExponentialMovingAverageOscillator(std::shared_ptr<LineRoot> data);
    virtual ~TripleExponentialMovingAverageOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // TEMA calculation variables
    double fast_ema1_, fast_ema2_, fast_ema3_;
    double slow_ema1_, slow_ema2_, slow_ema3_;
    
    // Smoothing factors
    double fast_alpha_, slow_alpha_;
    double fast_alpha1_, slow_alpha1_;
    
    bool first_run_;
};

// Aliases
using TEMAOsc = TripleExponentialMovingAverageOscillator;
using TEMAOSC = TripleExponentialMovingAverageOscillator;
using TEMAOscillator = TripleExponentialMovingAverageOscillator;

} // namespace indicators
} // namespace backtrader