#pragma once

#include "../indicator.h"
#include "../lineseries.h"
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
    TrueStrengthIndicator(std::shared_ptr<LineSeries> data_source, int period1 = 25, int period2 = 13);
    TrueStrengthIndicator(std::shared_ptr<LineRoot> data, int period1, int period2);
    virtual ~TrueStrengthIndicator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Storage for intermediate calculations
    std::vector<double> price_changes_;
    std::vector<double> abs_price_changes_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using TSI = TrueStrengthIndicator;

} // namespace backtrader