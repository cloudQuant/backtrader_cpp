#pragma once

#include "../indicator.h"
#include "sma.h"

namespace backtrader {

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
    virtual ~HeikinAshi() = default;
    
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
    std::shared_ptr<HeikinAshi> heikin_ashi_;
    std::shared_ptr<SMA> sma_;
};

// Aliases
using haD = HaDelta;

} // namespace backtrader