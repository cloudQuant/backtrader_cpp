#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <memory>

namespace backtrader {

// Forward declaration
class DataSeries;
namespace indicators {

// Williams Accumulation/Distribution Indicator
class WilliamsAccumulationDistribution : public Indicator {
public:
    struct Params {
        // No parameters for Williams A/D
    } params;
    
    // Line indices
    enum LineIndex {
        ad = 0  // Accumulation/Distribution line
    };
    
    WilliamsAccumulationDistribution();
    WilliamsAccumulationDistribution(std::shared_ptr<LineSeries> data_source);
    WilliamsAccumulationDistribution(std::shared_ptr<DataSeries> data_source);
    virtual ~WilliamsAccumulationDistribution() = default;
    
    // Utility methods
    double get(int ago = 0) const override;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Aliases
using WilliamsAD = WilliamsAccumulationDistribution;
using WAD = WilliamsAccumulationDistribution;

} // namespace indicators
} // namespace backtrader