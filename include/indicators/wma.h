#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"

namespace backtrader {
namespace indicators {

// Weighted Moving Average
class WeightedMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;
    } params;
    
    // Line indices
    enum LineIndex { 
        wma = 0  // Weighted Moving Average line
    };
    
    // Constructor with default parameters (original)
    WeightedMovingAverage();
    // Constructor with data source and period (Python-style API)
    WeightedMovingAverage(std::shared_ptr<LineSeries> data_source, int period = 30);
    WeightedMovingAverage(std::shared_ptr<DataSeries> data_source, int period = 30);
    virtual ~WeightedMovingAverage() = default;
    
    // Utility methods for tests
    double get(int ago = 0) const override;
    int getMinPeriod() const override { return params.period; }
    void calculate() override;
    size_t size() const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Precomputed weights and coefficient
    std::vector<double> weights_;
    double coef_;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using WMA = WeightedMovingAverage;
using MovingAverageWeighted = WeightedMovingAverage;

} // namespace indicators
} // namespace backtrader