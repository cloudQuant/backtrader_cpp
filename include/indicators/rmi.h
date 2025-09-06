#pragma once

#include "../indicator.h"
#include "../dataseries.h"
#include "smma.h"

namespace backtrader {

// Relative Momentum Index
class RelativeMomentumIndex : public Indicator {
public:
    struct Params {
        int period = 20;
        int lookback = 5;
    } params;
    
    // Lines
    enum Lines { 
        rmi = 0  // RMI line (alias for rsi)
    };
    
    RelativeMomentumIndex();
    RelativeMomentumIndex(std::shared_ptr<LineSeries> data_source);
    RelativeMomentumIndex(std::shared_ptr<LineSeries> data_source, int period, int lookback);
    RelativeMomentumIndex(std::shared_ptr<DataSeries> data_source);
    RelativeMomentumIndex(std::shared_ptr<DataSeries> data_source, int period, int lookback);
    virtual ~RelativeMomentumIndex() = default;
    
    // Utility methods for test framework
    double get(int ago = 0) const;
    size_t size() const;
    int getMinPeriod() const;
    void calculate();
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // SMMA for up and down calculations
    std::shared_ptr<indicators::SMMA> up_smma_;
    std::shared_ptr<indicators::SMMA> down_smma_;
    
    // Storage for up/down moves
    std::vector<double> up_moves_;
    std::vector<double> down_moves_;
    
    // Manual SMMA calculation state
    double up_smma_value_ = 0.0;
    double down_smma_value_ = 0.0;
    bool first_calc_ = true;
};

// Aliases
using RMI = RelativeMomentumIndex;

} // namespace backtrader