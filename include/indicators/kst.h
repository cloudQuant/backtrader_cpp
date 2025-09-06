#pragma once

#include "../indicator.h"
#include "sma.h"
#include "momentum.h"  // For ROC100
#include <memory>
#include <deque>

namespace backtrader {

// Know Sure Thing (KST) Indicator
class KnowSureThing : public Indicator {
public:
    struct Params {
        // ROC periods
        int rp1 = 10;
        int rp2 = 15;
        int rp3 = 20;
        int rp4 = 30;
        
        // Moving average periods for ROCs
        int rma1 = 10;
        int rma2 = 10;
        int rma3 = 10;
        int rma4 = 10;
        
        // Signal line period
        int rsignal = 9;
        
        // Factors for weighted sum
        std::vector<double> rfactors = {1.0, 2.0, 3.0, 4.0};
    } params;
    
    // Lines
    enum Lines { 
        kst = 0,     // KST line
        signal = 1   // Signal line
    };
    
    KnowSureThing();
    KnowSureThing(std::shared_ptr<DataSeries> data_source);
    KnowSureThing(std::shared_ptr<LineSeries> data_source);  // For test framework compatibility
    virtual ~KnowSureThing() = default;
    
    // Test framework compatibility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    std::shared_ptr<LineBuffer> getLine(int index) const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Storage for intermediate calculations
    std::deque<double> roc1_values_;
    std::deque<double> roc2_values_;
    std::deque<double> roc3_values_;
    std::deque<double> roc4_values_;
    std::deque<double> kst_values_;
};

// Aliases
using KST = KnowSureThing;

} // namespace backtrader