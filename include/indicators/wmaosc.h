#pragma once

#include "../indicator.h"
#include "../lineiterator.h"
#include "wma.h"
#include <memory>

namespace backtrader {
namespace indicators {

// WMA Oscillator indicator
class WMAOsc : public IndicatorBase {
public:
    struct Params {
        int period1 = 14;  // First WMA period
        int period2 = 28;  // Second WMA period
    } params;
    
    // Default constructor
    WMAOsc();
    
    // Constructor with parameters (LineIterator)
    WMAOsc(std::shared_ptr<LineIterator> data_line, 
           int period1 = 14, int period2 = 28);
    
    // Constructor with parameters (LineRoot)
    WMAOsc(std::shared_ptr<LineRoot> data_line, 
           int period1 = 14, int period2 = 28);
    
    virtual ~WMAOsc() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
    // Get minimum period required
    int getMinPeriod() const { return std::max(params.period1, params.period2); }
    
    // Calculate method for manual testing
    void calculate();
    
protected:
    
private:
    std::shared_ptr<WMA> wma1_;
    std::shared_ptr<WMA> wma2_;
};

} // namespace indicators
} // namespace backtrader