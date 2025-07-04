#pragma once

#include "../indicator.h"
#include "../lineseries.h"

namespace backtrader {
namespace indicators {

// True High Indicator
class TrueHigh : public Indicator {
public:
    // Lines
    enum Lines { 
        truehigh = 0
    };
    
    TrueHigh();
    virtual ~TrueHigh() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// True Low Indicator
class TrueLow : public Indicator {
public:
    // Lines
    enum Lines { 
        truelow = 0
    };
    
    TrueLow();
    virtual ~TrueLow() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// True Range Indicator
class TrueRange : public Indicator {
public:
    // Lines
    enum Lines { 
        tr = 0  // True Range
    };
    
    TrueRange();
    virtual ~TrueRange() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    double calculate_true_range(double high, double low, double prev_close) const;
};

// Average True Range Indicator
class AverageTrueRange : public Indicator {
public:
    struct Params {
        int period = 14;  // Period for smoothed moving average
        // Using smoothed moving average (Wilder's smoothing)
    } params;
    
    // Lines
    enum Lines { 
        atr = 0  // Average True Range
    };
    
    // Constructor with default parameters (original)
    AverageTrueRange();
    // Constructor with data source and period (Python-style API)
    AverageTrueRange(std::shared_ptr<LineSeries> data_source, int period = 14);
    virtual ~AverageTrueRange() = default;
    
    // Utility methods for tests
    double get(int ago = 0) const;
    int getMinPeriod() const { return params.period + 1; }
    void calculate();
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    double calculate_true_range(double high, double low, double prev_close) const;
    double calculate_smoothed_average(const std::vector<double>& values, int period) const;
    
    // For smoothed moving average calculation
    std::vector<double> tr_history_;
    double prev_atr_;
    bool first_calculation_;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using TR = TrueRange;
using ATR = AverageTrueRange;

} // namespace indicators
} // namespace backtrader