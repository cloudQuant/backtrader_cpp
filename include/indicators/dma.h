#pragma once

#include "../indicator.h"
#include "../lineseries.h"

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
    
    // Public calculation method
    void calculate() { next(); }
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Values for calculation
    double ema_value_;
    
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
        int displacement = 14;  // Added displacement parameter
        int gainlimit = 50;
    } params;
    
    // Lines
    enum Lines { 
        dma = 0  // Dickson Moving Average line
    };
    
    DicksonMovingAverage();
    // Constructor for test framework compatibility
    DicksonMovingAverage(std::shared_ptr<LineRoot> data);
    // Constructor with period and displacement for manual tests
    DicksonMovingAverage(std::shared_ptr<LineRoot> data, int period, int displacement);
    virtual ~DicksonMovingAverage() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<ZeroLagIndicator> zerolag_;
};

// Aliases
using DMA = DicksonMovingAverage;
using DicksonMA = DicksonMovingAverage;

} // namespace backtrader