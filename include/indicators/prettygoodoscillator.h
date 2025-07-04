#pragma once

#include "../indicator.h"
#include "sma.h"
#include "atr.h"

namespace backtrader {

// Pretty Good Oscillator
class PrettyGoodOscillator : public Indicator {
public:
    struct Params {
        int period = 14;
    } params;
    
    // Lines
    enum Lines { 
        pgo = 0  // Pretty Good Oscillator line
    };
    
    PrettyGoodOscillator();
    virtual ~PrettyGoodOscillator() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<SMA> sma_;
    std::shared_ptr<ATR> atr_;
};

// Aliases
using PGO = PrettyGoodOscillator;
using PrettyGoodOsc = PrettyGoodOscillator;

} // namespace backtrader