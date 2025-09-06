#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"

namespace backtrader {
namespace indicators {

// Forward declaration
class RateOfChange;

// Momentum Indicator
class Momentum : public Indicator {
public:
    struct Params {
        int period = 12;  // Period for momentum calculation
    } params;
    
    // Lines
    enum Lines { 
        momentum = 0
    };
    
    Momentum();
    Momentum(std::shared_ptr<LineSeries> data_source, int period = 12);
    // DataSeries constructor
    Momentum(std::shared_ptr<DataSeries> data_source, int period = 12);
    virtual ~Momentum() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Momentum Oscillator Indicator
class MomentumOscillator : public Indicator {
public:
    struct Params {
        int period = 12;     // Period for momentum calculation
        double band = 100.0; // Band for oscillator (similar to Python)
    } params;
    
    // Lines
    enum Lines { 
        momosc = 0  // Momentum Oscillator
    };
    
    MomentumOscillator();
    MomentumOscillator(std::shared_ptr<DataSeries> data_source, int period = 12);
    MomentumOscillator(std::shared_ptr<LineSeries> data_source, int period = 12);
    virtual ~MomentumOscillator() = default;
    
    // Test framework compatibility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::vector<double> momentum_values_;  // Instance variable for momentum values
};

// Rate of Change 100 Indicator (ROC * 100)
class RateOfChange100 : public Indicator {
public:
    struct Params {
        int period = 12;  // Period for ROC calculation
    } params;
    
    // Lines
    enum Lines { 
        roc100 = 0  // Rate of Change * 100
    };
    
    RateOfChange100();
    virtual ~RateOfChange100() = default;
    
    // Utility methods for test framework compatibility
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<indicators::RateOfChange> roc_indicator_;
};

// Aliases
using MomentumOsc = MomentumOscillator;
using ROC = RateOfChange;
using ROC100 = RateOfChange100;

} // namespace indicators
} // namespace backtrader