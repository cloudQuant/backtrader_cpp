#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
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
    // Constructor for test framework compatibility (single parameter)
    DoubleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source);
    // Constructor with period parameter
    DoubleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period);
    // DataSeries constructors
    DoubleExponentialMovingAverage(std::shared_ptr<DataSeries> data_source);
    DoubleExponentialMovingAverage(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~DoubleExponentialMovingAverage() = default;
    
    // Testing utility methods
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    void initialize_sub_indicators();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // Sub-indicators for DEMA calculation
    std::shared_ptr<EMA> ema1_;
    std::shared_ptr<EMA> ema2_;
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
    TripleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source);
    TripleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period);
    TripleExponentialMovingAverage(std::shared_ptr<DataSeries> data_source);
    TripleExponentialMovingAverage(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~TripleExponentialMovingAverage() = default;
    
    // Utility methods
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
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