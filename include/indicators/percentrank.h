#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <vector>

namespace backtrader {
namespace indicators {

// Percent Rank indicator
class PercentRank : public Indicator {
public:
    struct Params {
        int period = 50;  // Period for percent rank calculation
    } params;
    
    // Lines
    enum Lines { 
        pctrank = 0  // Percent rank line
    };
    
    PercentRank();
    PercentRank(std::shared_ptr<LineSeries> data_source, int period = 50);
    PercentRank(std::shared_ptr<DataSeries> data_source, int period = 50);
    virtual ~PercentRank() = default;
    
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
    
    // Helper method to calculate percent rank
    double calculate_percent_rank(const std::vector<double>& data, double current_value);
    
    // Circular buffer for period data
    std::vector<double> period_data_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using PctRank = PercentRank;

} // namespace indicators
} // namespace backtrader