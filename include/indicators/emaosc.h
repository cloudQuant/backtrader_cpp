#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Exponential Moving Average Oscillator
class ExponentialMovingAverageOscillator : public Indicator {
public:
    struct Params {
        int period = 30;   // EMA period for oscillator calculation (matches Python default)
    } params;
    
    // Lines
    enum Lines { 
        emaosc = 0  // EMA Oscillator line
    };
    
    ExponentialMovingAverageOscillator();
    ExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source);
    ExponentialMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period);
    // DataSeries constructors
    ExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source);
    ExponentialMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~ExponentialMovingAverageOscillator() = default;
    
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
    size_t current_index_;
    
    // EMA value
    double ema_;
    
    // Smoothing factors
    double alpha_, alpha1_;
    
    bool first_run_;
};

// Aliases
using EMAOsc = ExponentialMovingAverageOscillator;
using EMAOSC = ExponentialMovingAverageOscillator;
using EMAOscillator = ExponentialMovingAverageOscillator;

} // namespace indicators
} // namespace backtrader