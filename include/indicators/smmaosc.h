#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "smma.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Smoothed Moving Average Oscillator
// Oscillation of a SMMA around its data
// Formula: osc = data - SMMA(data, period)
class SmoothedMovingAverageOscillator : public Indicator {
public:
    struct Params {
        int period = 30;   // SMMA period
    } params;
    
    // Lines
    enum Lines { 
        smmaosc = 0  // SMMA Oscillator line
    };
    
    SmoothedMovingAverageOscillator();
    SmoothedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source);
    SmoothedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period);
    SmoothedMovingAverageOscillator(std::shared_ptr<DataSeries> data_source);
    SmoothedMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~SmoothedMovingAverageOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // SMMA indicator
    std::shared_ptr<indicators::SMMA> smma_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using SMMAOsc = SmoothedMovingAverageOscillator;
using SMMAOSC = SmoothedMovingAverageOscillator;
using SMMAOscillator = SmoothedMovingAverageOscillator;

} // namespace indicators
} // namespace backtrader