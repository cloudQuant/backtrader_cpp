#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
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
};

// Price Oscillator (Absolute)
class PriceOscillator : public PriceOscBase {
public:
    // Lines
    enum Lines { 
        po = 0  // Price Oscillator
    };
    
    PriceOscillator();
    PriceOscillator(std::shared_ptr<LineSeries> data_source);
    PriceOscillator(std::shared_ptr<DataSeries> data_source);
    PriceOscillator(std::shared_ptr<LineSeries> data_source, int period1, int period2);
    PriceOscillator(std::shared_ptr<DataSeries> data_source, int period1, int period2);
    virtual ~PriceOscillator() = default;
    
    // Utility methods for test framework
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate();
    size_t size() const;
    
protected:
    void setup_lines() override;
    void calculate_oscillator() override;
    void once(int start, int end) override;
    
private:
    // EMA calculation variables
    double ema1_value_;
    double ema2_value_;
    bool first_calculation_;
    double alpha1_;
    double alpha2_;
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
    PercentagePriceOscillator(std::shared_ptr<LineSeries> data_source);
    PercentagePriceOscillator(std::shared_ptr<LineSeries> data_source, int period1, int period2, int period_signal);
    PercentagePriceOscillator(std::shared_ptr<DataSeries> data_source, int period1 = 12, int period2 = 26, int period_signal = 9);
    virtual ~PercentagePriceOscillator() = default;
    
    // Utility methods for test framework
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate();
    
    // Line access methods for tests
    double getPPOLine(int ago = 0) const;
    double getSignalLine(int ago = 0) const;
    double getHistogramLine(int ago = 0) const;
    double getHistogram(int ago = 0) const;  // Alias for getHistogramLine
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    void setup_lines() override;
    void calculate_oscillator() override;
    
protected:
    bool use_long_denominator_;  // Use long MA as denominator if true, short MA if false
    
private:
    
    // EMA calculation variables
    double ema1_value_;
    double ema2_value_;
    double signal_ema_value_;
    bool first_calculation_;
    double alpha1_;
    double alpha2_;
    double alpha_signal_;
};

// Percentage Price Oscillator Short (uses short MA as denominator)
class PercentagePriceOscillatorShort : public PercentagePriceOscillator {
public:
    PercentagePriceOscillatorShort();
    PercentagePriceOscillatorShort(std::shared_ptr<LineSeries> data_source);
    PercentagePriceOscillatorShort(std::shared_ptr<DataSeries> data_source);
    PercentagePriceOscillatorShort(std::shared_ptr<LineSeries> data_source, int period1, int period2, int period_signal);
    PercentagePriceOscillatorShort(std::shared_ptr<DataSeries> data_source, int period1, int period2, int period_signal);
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