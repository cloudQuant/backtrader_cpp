#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "ema.h"
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
    DoubleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period = 30);
    virtual ~DoubleExponentialMovingAverage() = default;
    
    // Testing utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate();
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Sub-indicators
    std::shared_ptr<EMA> ema1_;  // First EMA
    std::shared_ptr<EMA> ema2_;  // EMA of EMA1
    
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
    virtual ~TripleExponentialMovingAverage() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Sub-indicators
    std::shared_ptr<EMA> ema1_;  // First EMA
    std::shared_ptr<EMA> ema2_;  // EMA of EMA1
    std::shared_ptr<EMA> ema3_;  // EMA of EMA2
};

// Weighted Moving Average (WMA)
class WeightedMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;  // Period for WMA calculation
    } params;
    
    // Lines
    enum Lines { 
        wma = 0
    };
    
    WeightedMovingAverage();
    virtual ~WeightedMovingAverage() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // WMA calculation coefficients
    double coef_;
    std::vector<double> weights_;
};

// Aliases
using DEMA = DoubleExponentialMovingAverage;
using TEMA = TripleExponentialMovingAverage;
using WMA = WeightedMovingAverage;
using MovingAverageDoubleExponential = DoubleExponentialMovingAverage;
using MovingAverageTripleExponential = TripleExponentialMovingAverage;
using MovingAverageWeighted = WeightedMovingAverage;

} // namespace indicators
} // namespace backtrader