#pragma once

#include "../indicator.h"
#include "../lineroot.h"
#include "../dataseries.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Fractal indicator - identifies fractal patterns in price data
class Fractal : public Indicator {
public:
    struct Params {
        int period = 5;  // Period for fractal identification
        double bardist = 0.015;  // Distance to max/min in absolute percentage
        int shift_to_potential_fractal = 2;  // Shift to potential fractal position
    } params;
    
    // Lines
    enum Lines { 
        up = 0,    // Up fractal line
        down = 1   // Down fractal line
    };
    
    Fractal();
    Fractal(std::shared_ptr<DataSeries> data_source);
    virtual ~Fractal() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    std::shared_ptr<LineSingle> getLine(size_t idx = 0) const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

} // namespace indicators
} // namespace backtrader