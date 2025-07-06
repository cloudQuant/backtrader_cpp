#pragma once

#include "../indicator.h"
#include "../lineroot.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Fractal indicator - identifies fractal patterns in price data
class Fractal : public Indicator {
public:
    struct Params {
        int period = 5;  // Period for fractal identification
    } params;
    
    // Lines
    enum Lines { 
        up = 0,    // Up fractal line
        down = 1   // Down fractal line
    };
    
    Fractal();
    Fractal(std::shared_ptr<LineRoot> data); // Constructor for test framework compatibility
    Fractal(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low, int period = 5);
    virtual ~Fractal() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

} // namespace indicators
} // namespace backtrader