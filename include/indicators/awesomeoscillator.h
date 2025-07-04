#pragma once

#include "../indicator.h"
#include "sma.h"
#include <memory>

namespace backtrader {

// Awesome Oscillator
class AwesomeOscillator : public Indicator {
public:
    struct Params {
        int fast = 5;     // Fast SMA period
        int slow = 34;    // Slow SMA period
    } params;
    
    // Lines
    enum Lines { 
        ao = 0  // Awesome Oscillator line
    };
    
    AwesomeOscillator();
    virtual ~AwesomeOscillator() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Sub-indicators
    std::shared_ptr<SMA> sma_fast_;  // Fast SMA of median price
    std::shared_ptr<SMA> sma_slow_;  // Slow SMA of median price
    
    // Storage for median price calculation
    std::vector<double> median_prices_;
};

// Aliases
using AwesomeOsc = AwesomeOscillator;
using AO = AwesomeOscillator;

} // namespace backtrader