#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <memory>
#include <deque>
#include <iostream>

namespace backtrader {
namespace indicators {

// Simple Moving Average Oscillator
class SimpleMovingAverageOscillator : public Indicator {
public:
    struct Params {
        int period = 30;   // SMA period for oscillator calculation
    } params;
    
    // Lines
    enum Lines { 
        smaosc = 0  // SMA Oscillator line
    };
    
    SimpleMovingAverageOscillator();
    SimpleMovingAverageOscillator(std::shared_ptr<LineSeries> data_source);
    SimpleMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period);
    // DataSeries constructor
    SimpleMovingAverageOscillator(std::shared_ptr<DataSeries> data_source);
    SimpleMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~SimpleMovingAverageOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const override;
    size_t size() const override;
    void calculate() override;
    int getMinPeriod() const override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // Price buffer for SMA calculation
    std::deque<double> price_buffer_;
    
    // Current sum
    double sum_;
    
    bool first_run_;
};

// Aliases
using SMAOsc = SimpleMovingAverageOscillator;
using SMAOSC = SimpleMovingAverageOscillator;
using SMAOscillator = SimpleMovingAverageOscillator;

} // namespace indicators
} // namespace backtrader