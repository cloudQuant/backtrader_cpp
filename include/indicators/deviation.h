#pragma once

#include "../indicator.h"
#include "sma.h"

namespace backtrader {

// Standard deviation indicator
class StandardDeviation : public Indicator {
public:
    struct Params {
        int period = 20;
        bool safepow = true;  // Use abs() to safeguard against negative values
    } params;
    
    // Lines
    enum Lines { 
        stddev = 0  // Standard deviation line
    };
    
    StandardDeviation();
    virtual ~StandardDeviation() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Moving averages for calculation
    std::shared_ptr<SMA> mean_sma_;
    std::shared_ptr<SMA> meansq_sma_;
};

// Mean deviation indicator
class MeanDeviation : public Indicator {
public:
    struct Params {
        int period = 20;
    } params;
    
    // Lines
    enum Lines { 
        meandev = 0  // Mean deviation line
    };
    
    MeanDeviation();
    virtual ~MeanDeviation() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Moving averages for calculation
    std::shared_ptr<SMA> mean_sma_;
    std::shared_ptr<SMA> absdev_sma_;
};

// Aliases
using StdDev = StandardDeviation;
using MeanDev = MeanDeviation;

} // namespace backtrader