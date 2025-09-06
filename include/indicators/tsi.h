#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
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
    TrueStrengthIndicator(std::shared_ptr<LineSeries> data_source);
    TrueStrengthIndicator(std::shared_ptr<LineSeries> data_source, int period1, int period2);
    TrueStrengthIndicator(std::shared_ptr<DataSeries> data_source);
    TrueStrengthIndicator(std::shared_ptr<DataSeries> data_source, int period1, int period2);
    virtual ~TrueStrengthIndicator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Aliases
using TSI = TrueStrengthIndicator;

} // namespace backtrader