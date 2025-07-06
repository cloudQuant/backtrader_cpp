#pragma once

#include "../indicator.h"
#include "ema.h"
#include <memory>

namespace backtrader {

// Base class for Price Oscillators
class PriceOscBase : public Indicator {
public:
    struct Params {
        int period1 = 12;  // Short period
        int period2 = 26;  // Long period
    } params;
    
    PriceOscBase();
    virtual ~PriceOscBase() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    virtual void setup_lines() = 0;
    virtual void calculate_oscillator() = 0;
    
    // Sub-indicators
    std::shared_ptr<indicators::EMA> ma1_;  // Short EMA
    std::shared_ptr<indicators::EMA> ma2_;  // Long EMA
};

// Price Oscillator (Absolute)
class PriceOscillator : public PriceOscBase {
public:
    // Lines
    enum Lines { 
        po = 0  // Price Oscillator
    };
    
    PriceOscillator();
    virtual ~PriceOscillator() = default;
    
protected:
    void setup_lines() override;
    void calculate_oscillator() override;
};

// Percentage Price Oscillator
class PercentagePriceOscillator : public PriceOscBase {
public:
    struct Params : public PriceOscBase::Params {
        int period_signal = 9;  // Signal line period
    } params;
    
    // Lines
    enum Lines { 
        ppo = 0,     // Percentage Price Oscillator
        signal = 1,  // Signal line
        histo = 2    // Histogram
    };
    
    PercentagePriceOscillator(bool use_long_denominator = true);
    virtual ~PercentagePriceOscillator() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    void setup_lines() override;
    void calculate_oscillator() override;
    
private:
    bool use_long_denominator_;  // Use long MA as denominator if true, short MA if false
    std::shared_ptr<indicators::EMA> signal_ema_;  // Signal line EMA
};

// Percentage Price Oscillator Short (uses short MA as denominator)
class PercentagePriceOscillatorShort : public PercentagePriceOscillator {
public:
    PercentagePriceOscillatorShort();
    virtual ~PercentagePriceOscillatorShort() = default;
};

// Aliases
using PriceOsc = PriceOscillator;
using AbsolutePriceOscillator = PriceOscillator;
using APO = PriceOscillator;
using AbsPriceOsc = PriceOscillator;
using PPO = PercentagePriceOscillator;
using PercPriceOsc = PercentagePriceOscillator;
using PPOShort = PercentagePriceOscillatorShort;
using PercPriceOscShort = PercentagePriceOscillatorShort;

} // namespace backtrader