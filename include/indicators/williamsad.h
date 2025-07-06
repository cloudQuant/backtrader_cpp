#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <memory>

namespace backtrader {
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
    WilliamsAccumulationDistribution(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low, 
                                   std::shared_ptr<LineRoot> close, std::shared_ptr<LineRoot> volume);
    virtual ~WilliamsAccumulationDistribution() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // LineRoot support (for multi-line constructor)
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<LineRoot> close_line_;
    std::shared_ptr<LineRoot> volume_line_;
    
    // Accumulation value
    double ad_value_;
};

// Aliases
using WilliamsAD = WilliamsAccumulationDistribution;
using WAD = WilliamsAccumulationDistribution;

} // namespace indicators
} // namespace backtrader