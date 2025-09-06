#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"

namespace backtrader {
namespace indicators {

// Percent Change indicator
class PercentChange : public Indicator {
public:
    struct Params {
        int period = 30;  // Period for percent change calculation
    } params;
    
    // Lines
    enum Lines { 
        pctchange = 0  // Percent change line
    };
    
    PercentChange();
    PercentChange(std::shared_ptr<LineSeries> data_source, int period = 30);
    PercentChange(std::shared_ptr<DataSeries> data_source, int period = 30);
    virtual ~PercentChange() = default;
    
    // Utility methods
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
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using PctChange = PercentChange;

} // namespace indicators
} // namespace backtrader