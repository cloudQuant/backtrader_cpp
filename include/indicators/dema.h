#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "ema.h"
#include "wma.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Double Exponential Moving Average (DEMA)
class DoubleExponentialMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;  // Period for DEMA calculation
    } params;
    
    // Lines
    enum Lines { 
        dema = 0
    };
    
    DoubleExponentialMovingAverage();
    // Constructor for test framework compatibility
    DoubleExponentialMovingAverage(std::shared_ptr<LineRoot> data);
    // Constructor for manual test compatibility
    DoubleExponentialMovingAverage(std::shared_ptr<LineRoot> data, int period);
    DoubleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period = 30);
    virtual ~DoubleExponentialMovingAverage() = default;
    
    // Testing utility methods
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Sub-indicators
    std::shared_ptr<indicators::EMA> ema1_;  // First EMA
    std::shared_ptr<indicators::EMA> ema2_;  // EMA of EMA1
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Triple Exponential Moving Average (TEMA)
class TripleExponentialMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;  // Period for TEMA calculation
    } params;
    
    // Lines
    enum Lines { 
        tema = 0
    };
    
    TripleExponentialMovingAverage();
    // Constructor for test framework compatibility
    TripleExponentialMovingAverage(std::shared_ptr<LineRoot> data);
    TripleExponentialMovingAverage(std::shared_ptr<LineRoot> data, int period);
    virtual ~TripleExponentialMovingAverage() = default;
    
    // Utility methods
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Sub-indicators
    std::shared_ptr<indicators::EMA> ema1_;  // First EMA
    std::shared_ptr<indicators::EMA> ema2_;  // EMA of EMA1
    std::shared_ptr<indicators::EMA> ema3_;  // EMA of EMA2
};

// WeightedMovingAverage is now included from wma.h

// Aliases
using DEMA = DoubleExponentialMovingAverage;
using TEMA = TripleExponentialMovingAverage;
using WMA = WeightedMovingAverage;
using MovingAverageDoubleExponential = DoubleExponentialMovingAverage;
using MovingAverageTripleExponential = TripleExponentialMovingAverage;
using MovingAverageWeighted = WeightedMovingAverage;

} // namespace indicators
} // namespace backtrader