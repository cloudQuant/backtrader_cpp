#pragma once

#include "../indicator.h"
#include <memory>

namespace backtrader {

// SAR Status structure for state tracking
struct SarStatus {
    double sar = 0.0;    // Stop and Reverse value
    bool tr = false;     // Trend direction (true = uptrend, false = downtrend)
    double af = 0.0;     // Acceleration Factor
    double ep = 0.0;     // Extreme Price (Significant Interest Point)
    
    SarStatus() = default;
    SarStatus(double sar_val, bool trend, double accel_factor, double extreme_price)
        : sar(sar_val), tr(trend), af(accel_factor), ep(extreme_price) {}
};

// Parabolic SAR (Stop and Reverse)
class ParabolicSAR : public Indicator {
public:
    struct Params {
        int period = 2;       // When to start showing values
        double af = 0.02;     // Initial acceleration factor
        double afmax = 0.20;  // Maximum acceleration factor
    } params;
    
    // Lines
    enum Lines { 
        psar = 0  // Parabolic SAR line
    };
    
    ParabolicSAR();
    virtual ~ParabolicSAR() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    size_t size() const override;
    
protected:
    void prenext() override;
    void nextstart() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Status tracking for SAR calculation
    std::array<SarStatus, 2> status_;
    bool initialized_;
};

// Aliases
using PSAR = ParabolicSAR;

} // namespace backtrader