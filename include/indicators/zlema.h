#pragma once

#include "../indicator.h"
#include "../dataseries.h"
#include "ema.h"

namespace backtrader {
namespace indicators {

// Zero Lag Exponential Moving Average
class ZeroLagExponentialMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;
    } params;
    
    // Line indices
    enum LineIndex { 
        zlema = 0  // Zero Lag EMA line
    };
    
    ZeroLagExponentialMovingAverage();
    ZeroLagExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period = 30);
    ZeroLagExponentialMovingAverage(std::shared_ptr<DataSeries> data_source, int period = 30);
    virtual ~ZeroLagExponentialMovingAverage() = default;
    
    // Utility methods
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    size_t size() const override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // EMA for calculation
    std::shared_ptr<indicators::EMA> ema_;
    
    // Lag value
    int lag_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using ZLEMA = ZeroLagExponentialMovingAverage;
using ZeroLagEma = ZeroLagExponentialMovingAverage;
using ZeroLagEMA = ZeroLagExponentialMovingAverage;

} // namespace indicators
} // namespace backtrader