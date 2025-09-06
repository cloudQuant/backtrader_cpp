#pragma once

#include "../indicator.h"
#include <vector>
#include <cmath>

namespace backtrader {

// Hurst Exponent Indicator
class HurstExponent : public Indicator {
public:
    struct Params {
        int period = 40;        // Default period (2000 recommended for stability)
        int lag_start = 2;      // Start lag (10 recommended)
        int lag_end = 0;        // End lag (0 means period/2, 500 recommended)
    } params;
    
    // Lines
    enum Lines { 
        hurst = 0  // Hurst exponent line
    };
    
    HurstExponent();
    virtual ~HurstExponent() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper method to calculate Hurst exponent
    double calculate_hurst(const std::vector<double>& data);
    
    // Helper method for linear regression
    std::pair<double, double> linear_regression(const std::vector<double>& x, 
                                                const std::vector<double>& y);
    
    // Lag range
    int lag_start_;
    int lag_end_;
    std::vector<int> lags_;
    std::vector<double> log10_lags_;
};

// Aliases
using Hurst = HurstExponent;

} // namespace backtrader