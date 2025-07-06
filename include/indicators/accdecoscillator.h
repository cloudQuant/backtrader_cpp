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
    AccelerationDecelerationOscillator(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low);
    virtual ~AccelerationDecelerationOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<AwesomeOscillator> awesome_oscillator_;
    
    // Storage for calculation
    std::vector<double> median_prices_;
    std::vector<double> ao_values_;
};

// Aliases
using AccDeOsc = AccelerationDecelerationOscillator;

} // namespace backtrader