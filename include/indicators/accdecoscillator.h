#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include "awesomeoscillator.h"
#include "sma.h"

namespace backtrader {

// Acceleration/Deceleration Oscillator
class AccelerationDecelerationOscillator : public Indicator {
public:
    struct Params {
        int period = 5;
    } params;
    
    // Lines
    enum Lines { 
        accde = 0  // Acceleration/Deceleration line
    };
    
    AccelerationDecelerationOscillator();
    AccelerationDecelerationOscillator(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low);
    AccelerationDecelerationOscillator(std::shared_ptr<LineSeries> data);
    AccelerationDecelerationOscillator(std::shared_ptr<DataSeries> data);
    virtual ~AccelerationDecelerationOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    
    // Calculate method for test framework
    void calculate() override;
    
    // Debug flag
    int calculate_called = 0;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<AwesomeOscillator> awesome_oscillator_;
    
    // Storage for calculation
    std::vector<double> median_prices_;
    std::vector<double> ao_values_;
    
    // Storage for high/low data from two-parameter constructor
    std::shared_ptr<LineSeries> high_data_;
    std::shared_ptr<LineSeries> low_data_;
};

// Aliases
using AccDeOsc = AccelerationDecelerationOscillator;

} // namespace backtrader