#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Triple Exponential Moving Average Oscillator
// TEMAOsc = data - TEMA(data, period)
class TripleExponentialMovingAverageOscillator : public Indicator {
public:
    struct Params {
        int period = 30;   // TEMA period (Python default is 30)
    } params;
    
    // Lines
    enum Lines { 
        temaosc = 0  // TEMA Oscillator line
    };
    
    TripleExponentialMovingAverageOscillator();
    TripleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source);
    TripleExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period);
    TripleExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source);
    TripleExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~TripleExponentialMovingAverageOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // TEMA indicator for calculation
    std::shared_ptr<class TripleExponentialMovingAverage> tema_;
};

// Aliases
using TEMAOsc = TripleExponentialMovingAverageOscillator;
using TEMAOSC = TripleExponentialMovingAverageOscillator;
using TEMAOscillator = TripleExponentialMovingAverageOscillator;

} // namespace indicators
} // namespace backtrader