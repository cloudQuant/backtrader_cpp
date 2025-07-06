#pragma once

#include "../indicator.h"

namespace backtrader {
namespace indicators {

// Base Oscillator class
class Oscillator : public Indicator {
public:
    // Lines
    enum LineIndex { 
        osc = 0  // Oscillator line
    };
    
    Oscillator();
    Oscillator(std::shared_ptr<LineSeries> data_source);
    Oscillator(std::shared_ptr<LineSeries> data_source, int period);
    Oscillator(std::shared_ptr<LineSeries> data_source, std::shared_ptr<Indicator> base_indicator);
    virtual ~Oscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    std::shared_ptr<Indicator> base_indicator_;
    size_t current_index_;
};

// SMA Oscillator
class SMAOscillator : public Indicator {
public:
    struct Params {
        int period = 30;
    } params;
    
    enum LineIndex { sma_osc = 0 };
    
    SMAOscillator();
    SMAOscillator(std::shared_ptr<LineRoot> data, int period = 30);
    virtual ~SMAOscillator() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// EMA Oscillator
class EMAOscillator : public Indicator {
public:
    struct Params {
        int period = 30;
    } params;
    
    enum LineIndex { ema_osc = 0 };
    
    EMAOscillator();
    EMAOscillator(std::shared_ptr<LineRoot> data, int period = 30);
    virtual ~EMAOscillator() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Aliases
using SMAOsc = SMAOscillator;
using EMAOsc = EMAOscillator;

} // namespace indicators
} // namespace backtrader