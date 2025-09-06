#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include "../linebuffer.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Forward declaration
class WeightedMovingAverage;

// Weighted Moving Average Oscillator
class WeightedMovingAverageOscillator : public Indicator {
public:
    struct Params {
        int period = 30;  // WMA period
    } params;
    
    // Lines
    enum Lines { 
        wmaosc = 0  // WMA Oscillator line
    };
    
    WeightedMovingAverageOscillator();
    WeightedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source);
    WeightedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period);
    // DataSeries constructors
    WeightedMovingAverageOscillator(std::shared_ptr<DataSeries> data_source);
    WeightedMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period);
    // LineBuffer constructors for tests
    WeightedMovingAverageOscillator(std::shared_ptr<LineBuffer> data);
    WeightedMovingAverageOscillator(std::shared_ptr<LineBuffer> data, int period);
    virtual ~WeightedMovingAverageOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    size_t size() const override;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    
    // WMA indicator
    std::shared_ptr<WeightedMovingAverage> wma_;
};

// Aliases
using WMAOsc = WeightedMovingAverageOscillator;
using WMAOSC = WeightedMovingAverageOscillator;
using WMAOscillator = WeightedMovingAverageOscillator;

} // namespace indicators
} // namespace backtrader