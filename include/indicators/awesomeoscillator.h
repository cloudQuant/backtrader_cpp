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
    AwesomeOscillator(std::shared_ptr<LineRoot> data); // Constructor for test framework compatibility
    AwesomeOscillator(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low);
    virtual ~AwesomeOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Storage for median price calculation
    std::vector<double> median_prices_;
};

// Aliases
using AwesomeOsc = AwesomeOscillator;
using AO = AwesomeOscillator;

} // namespace backtrader