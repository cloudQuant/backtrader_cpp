#pragma once

#include "kama.h"
#include "oscillator.h"

namespace backtrader {
namespace indicators {

// KAMA Oscillator - Combines two KAMA with different periods
class KAMAOscillator : public Indicator {
public:
    struct Params {
        int period1 = 10;    // Fast KAMA period
        int period2 = 30;    // Slow KAMA period
        int fast = 2;        // KAMA fast parameter
        int slow = 30;       // KAMA slow parameter
    } params;
    
    KAMAOscillator();
    KAMAOscillator(std::shared_ptr<LineRoot> data, int period1 = 10, int period2 = 30, int fast = 2, int slow = 30);
    virtual ~KAMAOscillator() = default;
    
    // Utility methods for test framework compatibility
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<KAMA> kama1_;  // Fast KAMA
    std::shared_ptr<KAMA> kama2_;  // Slow KAMA
};

// Aliases
using KamaOscillator = KAMAOscillator;
using KAMAOSC = KAMAOscillator;
using KAMAOsc = KAMAOscillator;

} // namespace indicators
} // namespace backtrader