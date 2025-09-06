#pragma once

#include "../indicator.h"
#include "../dataseries.h"
#include "sma.h"
#include <memory>

namespace backtrader {

// Awesome Oscillator
class AwesomeOscillator : public Indicator {
public:
    struct Params {
        int fast = 5;     // Fast SMA period
        int slow = 34;    // Slow SMA period
    } params;
    
    // Lines
    enum Lines { 
        ao = 0  // Awesome Oscillator line
    };
    
    AwesomeOscillator();
    AwesomeOscillator(std::shared_ptr<LineSeries> data_source); // Constructor for test framework LineSeries
    AwesomeOscillator(std::shared_ptr<DataSeries> data_source); // DataSeries constructor
    AwesomeOscillator(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low);
    virtual ~AwesomeOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    
    // Calculate method for test framework
    void calculate();
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Storage for median price calculation
    std::vector<double> median_prices_;
    
    // Direct storage for high/low data (LineSeries constructor)
    std::shared_ptr<LineSeries> high_data_;
    std::shared_ptr<LineSeries> low_data_;
};

// Aliases
using AwesomeOsc = AwesomeOscillator;
using AO = AwesomeOscillator;

} // namespace backtrader