#pragma once

#include "../indicator.h"
#include "ema.h"

namespace backtrader {
namespace indicators {

// Zero Lag Exponential Moving Average
class ZeroLagExponentialMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;
    } params;
    
    // Lines
    enum Lines { 
        zlema = 0  // Zero Lag EMA line
    };
    
    ZeroLagExponentialMovingAverage();
    ZeroLagExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period = 30);
    virtual ~ZeroLagExponentialMovingAverage() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate();
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // EMA for calculation
    std::shared_ptr<EMA> ema_;
    
    // Lag value
    int lag_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using ZLEMA = ZeroLagExponentialMovingAverage;
using ZeroLagEma = ZeroLagExponentialMovingAverage;

} // namespace indicators
} // namespace backtrader