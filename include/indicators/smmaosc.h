#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "smma.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Smoothed Moving Average Oscillator
class SmoothedMovingAverageOscillator : public Indicator {
public:
    struct Params {
        int fast = 15;   // Fast SMMA period
        int slow = 30;   // Slow SMMA period
    } params;
    
    // Lines
    enum Lines { 
        smmaosc = 0  // SMMA Oscillator line
    };
    
    SmoothedMovingAverageOscillator();
    SmoothedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source);
    SmoothedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int fast, int slow);
    // Constructor for test framework compatibility
    SmoothedMovingAverageOscillator(std::shared_ptr<LineRoot> data);
    SmoothedMovingAverageOscillator(std::shared_ptr<LineRoot> data, int fast, int slow);
    virtual ~SmoothedMovingAverageOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // SMMA indicators
    std::shared_ptr<indicators::SMMA> smma_fast_;
    std::shared_ptr<indicators::SMMA> smma_slow_;
    
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