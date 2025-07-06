#pragma once

#include "../indicator.h"
#include "sma.h"
#include <memory>

namespace backtrader {

// Base class for Stochastic indicators
class StochasticBase : public Indicator {
public:
    struct Params {
        int period = 14;          // Period for highest/lowest calculation
        int period_dfast = 3;     // Period for fast %D calculation
        double upperband = 80.0;  // Upper band level
        double lowerband = 20.0;  // Lower band level
        bool safediv = false;     // Use safe division
        double safezero = 0.0;    // Safe division zero value
    } params;
    
    // Lines
    enum Lines { 
        percK = 0,  // %K line
        percD = 1   // %D line
    };
    
    StochasticBase();
    virtual ~StochasticBase() = default;
    
    // Utility methods for test framework compatibility
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    virtual void setup_lines() = 0;
    virtual void calculate_lines() = 0;
    
    // Helper methods
    double get_highest(int period, int offset = 0);
    double get_lowest(int period, int offset = 0);
    
    // Sub-indicators
    std::shared_ptr<indicators::SMA> sma_fast_;
    std::shared_ptr<indicators::SMA> sma_slow_;
    
    // Intermediate values
    std::vector<double> k_values_;
    std::vector<double> d_values_;
};

// Fast Stochastic Oscillator
class StochasticFast : public StochasticBase {
public:
    StochasticFast();
    virtual ~StochasticFast() = default;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
};

// Regular (Slow) Stochastic Oscillator
class Stochastic : public StochasticBase {
public:
    struct Params : public StochasticBase::Params {
        int period_dslow = 3;  // Period for slow %D calculation
    } params;
    
    Stochastic();
    virtual ~Stochastic() = default;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
    
private:
    std::shared_ptr<indicators::SMA> sma_dslow_;
};

// Full Stochastic Oscillator (shows all three lines)
class StochasticFull : public StochasticBase {
public:
    struct Params : public StochasticBase::Params {
        int period_dslow = 3;  // Period for slow %D calculation
    } params;
    
    // Lines
    enum Lines { 
        percK = 0,     // %K line
        percD = 1,     // %D line
        percDSlow = 2  // %D Slow line
    };
    
    StochasticFull();
    virtual ~StochasticFull() = default;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
    
private:
    std::shared_ptr<indicators::SMA> sma_dslow_;
};

// Aliases
using StochasticSlow = Stochastic;

} // namespace backtrader