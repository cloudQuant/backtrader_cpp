#pragma once

#include "../indicator.h"

namespace backtrader {
namespace indicators {

// Forward declaration
class SMA;

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
    // Constructor for test framework compatibility
    Oscillator(std::shared_ptr<DataSeries> data_source, int period = 30);
    virtual ~Oscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    std::shared_ptr<Indicator> base_indicator_;
    std::shared_ptr<SMA> sma_indicator_;  // For single data source mode
    size_t current_index_;
    int period_;
};

// SMA Oscillator
class SMAOscillator : public Indicator {
public:
    struct Params {
        int period = 30;
    } params;
    
    enum LineIndex { sma_osc = 0 };
    
    SMAOscillator();
    // Constructor with data source
    SMAOscillator(std::shared_ptr<LineSeries> data_source, int period = 30);
    // Constructor with DataSeries for test framework compatibility
    SMAOscillator(std::shared_ptr<DataSeries> data_source, int period = 30);
    virtual ~SMAOscillator() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    size_t size() const override;
    
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
    // Constructor with data source
    EMAOscillator(std::shared_ptr<LineSeries> data_source, int period = 30);
    // Constructor with DataSeries for test framework compatibility
    EMAOscillator(std::shared_ptr<DataSeries> data_source, int period = 30);
    virtual ~EMAOscillator() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    size_t size() const override;
    
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
