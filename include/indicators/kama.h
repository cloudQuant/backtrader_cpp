#pragma once

#include "../indicator.h"
#include "sma.h"
#include <memory>

namespace backtrader {

// Kaufman's Adaptive Moving Average (KAMA)
class AdaptiveMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;  // Period for efficiency ratio calculation
        int fast = 2;     // Fast EMA period
        int slow = 30;    // Slow EMA period
    } params;
    
    // Lines
    enum Lines { 
        kama = 0
    };
    
    AdaptiveMovingAverage();
    // Constructor for test framework compatibility  
    AdaptiveMovingAverage(std::shared_ptr<LineRoot> data, int period = 30, int fast = 2, int slow = 30);
    virtual ~AdaptiveMovingAverage() = default;
    
    // Utility methods for test framework compatibility
    double get(int ago = 0) const;
    int getMinPeriod() const;
    
protected:
    void prenext() override;
    void nextstart() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper methods
    double calculate_efficiency_ratio();
    double calculate_smoothing_constant(double efficiency_ratio);
    
    // Initial seed calculation
    std::shared_ptr<indicators::SMA> sma_seed_;
    
    // Previous KAMA value for recursive calculation
    double prev_kama_;
    bool initialized_;
    
    // Smoothing factors
    double fast_sc_;  // Fast smoothing constant
    double slow_sc_;  // Slow smoothing constant
    
    // Circular buffer for volatility calculation
    std::vector<double> price_changes_;
};

// Aliases
using KAMA = AdaptiveMovingAverage;
using MovingAverageAdaptive = AdaptiveMovingAverage;

} // namespace backtrader