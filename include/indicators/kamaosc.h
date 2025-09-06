#pragma once

#include "kama.h"
#include "oscillator.h"

namespace backtrader {
namespace indicators {

// KAMA Oscillator - Calculates oscillator as data - KAMA(data)
class KAMAOscillator : public Indicator {
public:
    struct Params {
        int period1 = 30;    // KAMA period (default 30 to match Python)
        int period2 = 30;    // Not used (kept for compatibility)
        int fast = 2;        // KAMA fast parameter
        int slow = 30;       // KAMA slow parameter
    } params;
    
    KAMAOscillator();
    KAMAOscillator(std::shared_ptr<LineSeries> data, int period1 = 30, int period2 = 30, int fast = 2, int slow = 30);
    KAMAOscillator(std::shared_ptr<DataSeries> data, int period1 = 30, int period2 = 30, int fast = 2, int slow = 30);
    virtual ~KAMAOscillator() = default;
    
    // Utility methods for test framework compatibility
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<KAMA> kama1_;  // KAMA indicator
    std::shared_ptr<KAMA> kama2_;  // Not used (kept for compatibility)
};

// Aliases
using KamaOscillator = KAMAOscillator;
using KAMAOSC = KAMAOscillator;
using KAMAOsc = KAMAOscillator;

} // namespace indicators
} // namespace backtrader