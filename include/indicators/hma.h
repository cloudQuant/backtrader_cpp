#pragma once

#include "../indicator.h"
#include "../lineseries.h"
// #include "dema.h"  // For WMA - removed dependency
#include <memory>

namespace backtrader {
namespace indicators {

// Hull Moving Average
class HullMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;  // Period for HMA calculation
    } params;
    
    // Lines
    enum Lines { 
        hma = 0
    };
    
    HullMovingAverage();
    HullMovingAverage(std::shared_ptr<LineSeries> data_source, int period = 30);
    // Constructor for test framework compatibility
    HullMovingAverage(std::shared_ptr<LineRoot> data, int period = 30);
    virtual ~HullMovingAverage() = default;
    
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
    
    // Price history for simple calculation
    std::vector<double> prices_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using HMA = HullMovingAverage;
using HullMA = HullMovingAverage;

} // namespace indicators
} // namespace backtrader