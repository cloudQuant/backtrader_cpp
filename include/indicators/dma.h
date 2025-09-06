#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include "zlind.h"
#include "hma.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Dickson Moving Average - combines ZeroLagIndicator and HullMovingAverage
// Formula: DMA = (EC + HMA) / 2
class DicksonMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;        // Period for ZeroLagIndicator
        int gainlimit = 50;     // Gain limit for ZeroLagIndicator
        int hperiod = 7;        // Period for HullMovingAverage (default from Python)
    } params;
    
    // Lines
    enum Lines { 
        dma = 0  // Dickson Moving Average line
    };
    
    DicksonMovingAverage();
    DicksonMovingAverage(std::shared_ptr<LineSeries> data_source, int period = 30);
    // DataSeries constructors for test framework compatibility
    DicksonMovingAverage(std::shared_ptr<DataSeries> data_source);
    DicksonMovingAverage(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~DicksonMovingAverage() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Internal indicators
    std::shared_ptr<ZeroLagIndicator> zlind_;
    std::shared_ptr<HullMovingAverage> hma_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
};

// Aliases
using DMA = DicksonMovingAverage;

} // namespace indicators
} // namespace backtrader