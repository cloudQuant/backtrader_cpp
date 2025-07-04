#pragma once

#include "../indicator.h"
#include "../lineseries.h"

namespace backtrader {
namespace indicators {

// MACD (Moving Average Convergence Divergence) Indicator
class MACD : public Indicator {
public:
    struct Params {
        int period_me1 = 12;        // Fast EMA period
        int period_me2 = 26;        // Slow EMA period  
        int period_signal = 9;      // Signal line EMA period
        // For simplicity, always use EMA (can be extended later)
    } params;
    
    // Lines
    enum Lines { 
        macd = 0,
        signal = 1
    };
    
    // Constructor with default parameters (original)
    MACD();
    // Constructor with data source and parameters (Python-style API)
    MACD(std::shared_ptr<LineSeries> data_source, int fast_period = 12, int slow_period = 26, int signal_period = 9);
    virtual ~MACD() = default;
    
    // Utility methods for tests
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate();
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    std::shared_ptr<Indicator> me1_;  // Fast EMA
    std::shared_ptr<Indicator> me2_;  // Slow EMA
    std::shared_ptr<Indicator> signal_ema_;  // Signal line EMA
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    void setup_lines();
    void calculate_macd();
    void calculate_signal();
};

// MACD Histogram - extends MACD with histogram line
class MACDHisto : public MACD {
public:
    // Additional line for histogram
    enum Lines { 
        macd = 0,
        signal = 1,
        histo = 2
    };
    
    MACDHisto();
    virtual ~MACDHisto() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void calculate_histogram();
};

// Alias
using MACDHistogram = MACDHisto;

} // namespace indicators
} // namespace backtrader