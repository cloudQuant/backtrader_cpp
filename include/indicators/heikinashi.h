#pragma once

#include "../indicator.h"
#include "sma.h"

namespace backtrader {
namespace indicators {

// Heikin Ashi Indicator
class HeikinAshi : public Indicator {
public:
    // Lines
    enum Lines {
        ha_open = 0,
        ha_high = 1,
        ha_low = 2,
        ha_close = 3
    };
    
    HeikinAshi();
    // Constructor for test framework compatibility
    HeikinAshi(std::shared_ptr<LineRoot> data, int period = 1);
    // Constructor for OHLC data lines
    HeikinAshi(std::shared_ptr<LineRoot> open_line, std::shared_ptr<LineRoot> high_line, 
               std::shared_ptr<LineRoot> low_line, std::shared_ptr<LineRoot> close_line);
    virtual ~HeikinAshi() = default;
    
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
};

// Heikin Ashi Delta Indicator
class HaDelta : public Indicator {
public:
    struct Params {
        int period = 3;
        bool autoheikin = true;  // Automatically apply Heikin Ashi transformation
    } params;
    
    // Lines
    enum Lines {
        haDelta = 0,
        smoothed = 1
    };
    
    HaDelta();
    virtual ~HaDelta() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<indicators::HeikinAshi> heikin_ashi_;
    std::shared_ptr<indicators::SMA> sma_;
};

// Aliases
using haD = HaDelta;

} // namespace indicators
} // namespace backtrader