#pragma once

#include "../indicator.h"
#include "../lineseries.h"

namespace backtrader {
namespace indicators {

// Zero Lag Indicator (Error Correcting)
class ZeroLagIndicator : public Indicator {
public:
    struct Params {
        int period = 14;
        int gainlimit = 50;
    } params;
    
    // Line indices
    enum LineIndex { 
        ec = 0  // Error correcting line
    };
    
    ZeroLagIndicator();
    ZeroLagIndicator(std::shared_ptr<LineSeries> data_source, int period = 14);
    ZeroLagIndicator(std::shared_ptr<LineRoot> data_source, int period = 14);
    virtual ~ZeroLagIndicator() = default;
    
    // Utility methods
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Alpha values for calculation
    double alpha_;
    double alpha1_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // LineRoot support (for LineRoot constructor)
    std::shared_ptr<LineRoot> lineroot_source_;
};

// Aliases
using ZLIndicator = ZeroLagIndicator;
using ZLInd = ZeroLagIndicator;
using EC = ZeroLagIndicator;
using ErrorCorrecting = ZeroLagIndicator;

} // namespace indicators
} // namespace backtrader