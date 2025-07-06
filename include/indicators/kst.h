#pragma once

#include "../indicator.h"
#include "sma.h"
#include "momentum.h"  // For ROC100

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
    virtual ~KnowSureThing() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<indicators::RateOfChange100> roc1_, roc2_, roc3_, roc4_;
    std::shared_ptr<indicators::SMA> rcma1_, rcma2_, rcma3_, rcma4_;
    std::shared_ptr<indicators::SMA> signal_sma_;
};

// Aliases
using KST = KnowSureThing;

} // namespace backtrader