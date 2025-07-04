#pragma once

#include "../indicator.h"
#include "atr.h"
#include <memory>

namespace backtrader {

// Williams %R indicator
class WilliamsR : public Indicator {
public:
    struct Params {
        int period = 14;          // Period for calculation
        double upperband = -20.0; // Upper band level
        double lowerband = -80.0; // Lower band level
    } params;
    
    // Lines
    enum Lines { 
        percR = 0  // Williams %R line
    };
    
    WilliamsR();
    virtual ~WilliamsR() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper methods
    double get_highest(int period);
    double get_lowest(int period);
};

// Williams Accumulation/Distribution indicator
class WilliamsAD : public Indicator {
public:
    // Lines
    enum Lines { 
        ad = 0  // Accumulation/Distribution line
    };
    
    WilliamsAD();
    virtual ~WilliamsAD() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper methods
    bool is_upday(double current_close, double prev_close);
    bool is_downday(double current_close, double prev_close);
    
    // Sub-indicators for TrueHigh and TrueLow
    std::shared_ptr<TrueHigh> truehigh_;
    std::shared_ptr<TrueLow> truelow_;
    
    // Accumulation value
    double accumulated_ad_;
};

// Aliases
using WilliamsPercentR = WilliamsR;

} // namespace backtrader