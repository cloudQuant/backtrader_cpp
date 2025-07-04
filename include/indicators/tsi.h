#pragma once

#include "../indicator.h"
#include "ema.h"
#include <memory>

namespace backtrader {

// True Strength Indicator (TSI)
class TrueStrengthIndicator : public Indicator {
public:
    struct Params {
        int period1 = 25;   // Period for first smoothing
        int period2 = 13;   // Period for second smoothing  
        int pchange = 1;    // Lookback period for price change
    } params;
    
    // Lines
    enum Lines { 
        tsi = 0
    };
    
    TrueStrengthIndicator();
    virtual ~TrueStrengthIndicator() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Sub-indicators for double smoothing
    std::shared_ptr<EMA> sm1_;   // First smoothing of price change
    std::shared_ptr<EMA> sm12_;  // Second smoothing of price change
    std::shared_ptr<EMA> sm2_;   // First smoothing of absolute price change
    std::shared_ptr<EMA> sm22_;  // Second smoothing of absolute price change
    
    // Storage for intermediate calculations
    std::vector<double> price_changes_;
    std::vector<double> abs_price_changes_;
};

// Aliases
using TSI = TrueStrengthIndicator;

} // namespace backtrader