#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <memory>
#include <deque>

namespace backtrader {
namespace indicators {

// Simple Moving Average Oscillator
class SimpleMovingAverageOscillator : public Indicator {
public:
    struct Params {
        int period = 14;   // SMA period for oscillator calculation
    } params;
    
    // Lines
    enum Lines { 
        smaosc = 0  // SMA Oscillator line
    };
    
    SimpleMovingAverageOscillator();
    SimpleMovingAverageOscillator(std::shared_ptr<LineSeries> data_source);
    SimpleMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, int period);
    // Constructor for test framework compatibility
    SimpleMovingAverageOscillator(std::shared_ptr<LineRoot> data);
    SimpleMovingAverageOscillator(std::shared_ptr<LineRoot> data, int period);
    // Two-period constructor for compatibility
    SimpleMovingAverageOscillator(std::shared_ptr<LineRoot> data, int fast, int slow);
    virtual ~SimpleMovingAverageOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
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