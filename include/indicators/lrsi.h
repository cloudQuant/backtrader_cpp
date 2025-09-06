#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"

namespace backtrader {
namespace indicators {

// Laguerre RSI indicator  
class LaguerreRSI : public Indicator {
public:
    struct Params {
        double gamma = 0.5;  // Gamma parameter (0.2 to 0.8)
        int period = 6;      // Period for calculation
    } params;
    
    // Lines
    enum Lines { 
        lrsi = 0
    };
    
    LaguerreRSI();
    // Constructor for test framework compatibility with LineSeries
    LaguerreRSI(std::shared_ptr<LineSeries> data_source, double gamma = 0.5);
    // Constructor for test framework compatibility with DataSeries
    LaguerreRSI(std::shared_ptr<DataSeries> data_source, double gamma = 0.5);
    virtual ~LaguerreRSI() = default;
    
    // Get method for accessing the indicator value
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
    
    // Laguerre filter intermediate values
    double l0_, l1_, l2_, l3_;
};

// Laguerre Filter indicator
class LaguerreFilter : public Indicator {
public:
    struct Params {
        double gamma = 0.5;  // Gamma parameter (0.2 to 0.8)
    } params;
    
    // Lines
    enum Lines { 
        lfilter = 0
    };
    
    LaguerreFilter();
    virtual ~LaguerreFilter() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Laguerre filter intermediate values
    double l0_, l1_, l2_, l3_;
};

// Aliases
using LRSI = LaguerreRSI;
using LAGF = LaguerreFilter;

} // namespace indicators
} // namespace backtrader