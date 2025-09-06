#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "sma.h"
#include "percentrank.h"

namespace backtrader {
namespace indicators {

// DV2 Indicator (RSI alternative)
class DV2 : public Indicator {
public:
    struct Params {
        int period = 252;      // Period for percent rank calculation
        int maperiod = 2;      // Period for moving average
    } params;
    
    // Lines
    enum Lines { 
        dv2 = 0  // DV2 line
    };
    
    DV2();
    // DataSeries constructor for test framework compatibility
    DV2(std::shared_ptr<DataSeries> data_source, int period = 252);
    virtual ~DV2() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<indicators::SMA> sma_;
    std::shared_ptr<indicators::PercentRank> percent_rank_;
    
    // Intermediate data storage
    std::vector<double> chl_values_;
    std::vector<double> dvu_values_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

} // namespace indicators
} // namespace backtrader