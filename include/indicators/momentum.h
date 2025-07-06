#pragma once

#include "../indicator.h"
#include "../lineseries.h"

namespace backtrader {
namespace indicators {

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
    // Constructor for test framework compatibility
    Momentum(std::shared_ptr<LineRoot> data);
    Momentum(std::shared_ptr<LineRoot> data, int period);
    virtual ~Momentum() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
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
        double band = 100.0; // Reference band level
    } params;
    
    // Lines
    enum Lines { 
        momosc = 0  // Momentum Oscillator
    };
    
    MomentumOscillator();
    virtual ~MomentumOscillator() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Rate of Change Indicator
class RateOfChange : public Indicator {
public:
    struct Params {
        int period = 12;  // Period for ROC calculation
    } params;
    
    // Lines
    enum Lines { 
        roc = 0  // Rate of Change
    };
    
    RateOfChange();
    RateOfChange(std::shared_ptr<LineSeries> data_source, int period = 12);
    virtual ~RateOfChange() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
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