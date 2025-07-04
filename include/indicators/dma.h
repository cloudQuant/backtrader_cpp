#pragma once

#include "../indicator.h"
#include "ema.h"
#include "hma.h"

namespace backtrader {

// Zero Lag Indicator (Error Correcting)
class ZeroLagIndicator : public Indicator {
public:
    struct Params {
        int period = 14;
        int gainlimit = 50;
    } params;
    
    // Lines
    enum Lines { 
        zerolag = 0  // Zero lag line
    };
    
    ZeroLagIndicator();
    virtual ~ZeroLagIndicator() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // EMA for calculation
    std::shared_ptr<EMA> ema_;
    
    // Alpha values
    double alpha_;
    double alpha1_;
    
    // Error correction variables
    double ec_;
    double ec1_;
    double ec2_;
};

// Dickson Moving Average
class DicksonMovingAverage : public Indicator {
public:
    struct Params {
        int period = 14;
        int gainlimit = 50;
        int hperiod = 7;
    } params;
    
    // Lines
    enum Lines { 
        dma = 0  // Dickson Moving Average line
    };
    
    DicksonMovingAverage();
    virtual ~DicksonMovingAverage() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<ZeroLagIndicator> zerolag_;
    std::shared_ptr<HMA> hma_;
};

// Aliases
using DMA = DicksonMovingAverage;
using DicksonMA = DicksonMovingAverage;

} // namespace backtrader