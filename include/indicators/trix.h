#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "ema.h"
#include <memory>

namespace backtrader {
namespace indicators {

// TRIX indicator
class Trix : public Indicator {
public:
    struct Params {
        int period = 15;       // Period for triple EMA
        int _rocperiod = 1;    // Rate of change period
    } params;
    
    // Lines
    enum Lines { 
        trix = 0
    };
    
    Trix();
    Trix(std::shared_ptr<LineSeries> data_source, int period = 15);
    Trix(std::shared_ptr<LineRoot> data, int period = 15);
    virtual ~Trix() = default;
    
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
    
    // Sub-indicators for triple EMA smoothing
    std::shared_ptr<indicators::EMA> ema1_;  // First EMA
    std::shared_ptr<indicators::EMA> ema2_;  // Second EMA (EMA of EMA1)
    std::shared_ptr<indicators::EMA> ema3_;  // Third EMA (EMA of EMA2)
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// TRIX with Signal Line
class TrixSignal : public Trix {
public:
    struct Params : public Trix::Params {
        int sigperiod = 9;  // Signal line period
    } params;
    
    // Lines
    enum Lines { 
        trix = 0,
        signal = 1
    };
    
    TrixSignal();
    virtual ~TrixSignal() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines_signal();
    
    // Signal line EMA
    std::shared_ptr<indicators::EMA> signal_ema_;
};

// Aliases
using TRIX = Trix;

} // namespace indicators
} // namespace backtrader