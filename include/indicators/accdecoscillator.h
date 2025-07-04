#pragma once

#include "../indicator.h"
#include "awesomeoscillator.h"
#include "sma.h"

namespace backtrader {

// Acceleration/Deceleration Oscillator
class AccelerationDecelerationOscillator : public Indicator {
public:
    struct Params {
        int period = 5;
    } params;
    
    // Lines
    enum Lines { 
        accde = 0  // Acceleration/Deceleration line
    };
    
    AccelerationDecelerationOscillator();
    virtual ~AccelerationDecelerationOscillator() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<AwesomeOscillator> awesome_oscillator_;
    std::shared_ptr<SMA> sma_;
};

// Aliases
using AccDeOsc = AccelerationDecelerationOscillator;

} // namespace backtrader