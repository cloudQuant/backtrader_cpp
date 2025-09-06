#pragma once

#include "../indicator.h"
#include "atr.h"
#include <memory>

namespace backtrader {

// Williams %R indicator
class WilliamsR : public Indicator {
public:
    struct Params {
        int period = 14;          // Period for calculation
        double upperband = -20.0; // Upper band level
        double lowerband = -80.0; // Lower band level
    } params;
    
    // Lines
    enum Lines { 
        percR = 0  // Williams %R line
    };
    
    WilliamsR();
    virtual ~WilliamsR() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper methods
    double get_highest(int period);
    double get_lowest(int period);
};

// Williams Accumulation/Distribution indicator - see williamsad.h for implementation

// Aliases
using WilliamsPercentR = WilliamsR;

} // namespace backtrader