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
    PrettyGoodOscillator(std::shared_ptr<LineRoot> data_source, int period = 14);
    virtual ~PrettyGoodOscillator() = default;
    
    // Utility methods required by test framework
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<indicators::SMA> sma_;
    std::shared_ptr<indicators::ATR> atr_;
};

// Aliases
using PGO = PrettyGoodOscillator;
using PrettyGoodOsc = PrettyGoodOscillator;

} // namespace backtrader